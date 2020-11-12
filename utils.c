
#include <jni.h>
#include <android/log.h>

#define LOGE(...)   ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))
#define LOG_TAG "kill-tag"

char *jstringToChar(JNIEnv *env, jstring jstr) {
    char *rtn = NULL;
    jclass clsstring = env->FindClass("java/lang/String");
    jstring strencode = env->NewStringUTF("GB2312");
    jmethodID mid = env->GetMethodID(clsstring, "getBytes", "(Ljava/lang/String;)[B");
    jbyteArray barr = (jbyteArray) env->CallObjectMethod(jstr, mid, strencode);
    jsize alen = env->GetArrayLength(barr);
    jbyte *ba = env->GetByteArrayElements(barr, JNI_FALSE);
    if (alen > 0) {
        rtn = (char *) malloc(alen + 1);
        memcpy(rtn, ba, alen);
        rtn[alen] = 0;
    }
    env->ReleaseByteArrayElements(barr, ba, 0);
    return rtn;
}

// clear activity stack in Android for killing process
JNICALL void clear_activity_stack(JNIEnv *env, jclass cls, jstring pkgName) {
    LOGE("clearAcStack method enter, pkgName : %s", jstringToChar(env, pkgName));
    // 获取ActivityThread 对象
    jclass clazz = env->FindClass("android/app/ActivityThread");
    jmethodID m_currentAT = env->GetStaticMethodID(clazz, "currentActivityThread",
                                                   "()Landroid/app/ActivityThread;");
    jobject obj_at = env->CallStaticObjectMethod(clazz, m_currentAT);

    // 获取ContextImpl 对象
    jmethodID m_getSystemContext = env->GetMethodID(clazz, "getSystemContext",
                                                    "()Landroid/app/ContextImpl;");
    jobject obj_contextImpl = env->CallObjectMethod(obj_at, m_getSystemContext);

    // 创建属于应用的context对象, 否则是系统的context, 获取包名将返回Android xref: /frameworks/base/core/java/android/app/ContextImpl.java
    jclass clazz_contextImpl = env->FindClass("android/app/ContextImpl");
    jmethodID m_createPackageContext = env->GetMethodID(clazz_contextImpl, "createPackageContext",
                                                        "(Ljava/lang/String;I)Landroid/content/Context;");
    obj_contextImpl = env->CallObjectMethod(obj_contextImpl, m_createPackageContext, pkgName, 2);

    // 获取PackageManager 对象
    jmethodID m_getPackageManager = env->GetMethodID(clazz_contextImpl, "getPackageManager",
                                      "()Landroid/content/pm/PackageManager;");
    jobject obj_packageManager = env->CallObjectMethod(obj_contextImpl, m_getPackageManager);

    // 获取 Intent 对象
    jclass clazz_packageManager = env->GetObjectClass(obj_packageManager);
    jmethodID m_getLIFP = env->GetMethodID(clazz_packageManager, "getLaunchIntentForPackage",
                                      "(Ljava/lang/String;)Landroid/content/Intent;");
    jobject obj_intent = env->CallObjectMethod(obj_packageManager, m_getLIFP, pkgName);

    if (obj_intent == NULL) {
        LOGE("clearAcStack getLaunchIntentForPackage ret is null, pkgname : %s",
             jstringToChar(env, pkgName));
        return;
    }

    // 获取 ComponentName 对象 ComponentName cn = intent.getComponent();
    jclass clazz_intent = env->FindClass("android/content/Intent");
    jmethodID m_getComponent = env->GetMethodID(clazz_intent, "getComponent", "()Landroid/content/ComponentName;");
    jobject obj_componentName = env->CallObjectMethod(obj_intent, m_getComponent);

    // Intent mainIntent = Intent.makeRestartActivityTask(cn);
    jmethodID m_makeRestartAT = env->GetStaticMethodID(clazz_intent, "makeRestartActivityTask",
                                            "(Landroid/content/ComponentName;)Landroid/content/Intent;");
    jobject obj_mainIntent = env->CallStaticObjectMethod(clazz_intent, m_makeRestartAT, obj_componentName);

    // context.startActivity(mainIntent);
    jmethodID m_startActivity = env->GetMethodID(clazz_contextImpl, "startActivity", "(Landroid/content/Intent;)V");
    env->CallVoidMethod(obj_contextImpl, m_startActivity, obj_mainIntent);

    // intent = new Intent(Intent.ACTION_MAIN);
    jstring str1 = env->NewStringUTF("android.intent.action.MAIN");
    jmethodID mth9 = env->GetMethodID(clazz_intent, "<init>", "(Ljava/lang/String;)V");
    obj_intent = env->NewObject(clazz_intent, mth9, str1);

    // intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP);
    jmethodID mth10 = env->GetMethodID(clazz_intent, "setFlags", "(I)Landroid/content/Intent;");

    jfieldID flag_new_task = env->GetStaticFieldID(clazz_intent, "FLAG_ACTIVITY_NEW_TASK", "I");
    jfieldID flag_clear_top = env->GetStaticFieldID(clazz_intent, "FLAG_ACTIVITY_CLEAR_TOP", "I");

    int flag_1 = env->GetStaticIntField(clazz_intent, flag_new_task);
    int flag_2 = env->GetStaticIntField(clazz_intent, flag_clear_top);
    obj_intent = env->CallObjectMethod(obj_intent, mth10, (flag_1 | flag_2));

    // intent.addCategory(Intent.CATEGORY_HOME);
    jmethodID mth11 = env->GetMethodID(clazz_intent, "addCategory",
                                       "(Ljava/lang/String;)Landroid/content/Intent;");
    jstring str2 = env->NewStringUTF("android.intent.category.HOME");
    obj_intent = env->CallObjectMethod(obj_intent, mth11, str2);

    // context.startActivity(intent);
    env->CallVoidMethod(obj_contextImpl, m_startActivity, obj_intent);

    // System.exit(0);
    jclass clazz_system = env->FindClass("java/lang/System");
    jmethodID m_exit = env->GetStaticMethodID(clazz_system, "exit", "(I)V");
    env->CallStaticVoidMethod(clazz_system, m_exit, 0);

    env->DeleteLocalRef(obj_intent);
    env->DeleteLocalRef(str1);
    env->DeleteLocalRef(str2);

}


static inline void
setJniMethodTable(const char *name, const char *args, void *func, JNINativeMethod *method) {
    method->name = name;
    method->signature = args;
    method->fnPtr = func;
}

extern "C" jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = NULL;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        return JNI_ERR;
    }

    jclass clazz = env->FindClass("com/xxx/xxx/JNIBridge"); //类路径自己改
    JNINativeMethod jniNativeMethod[1];
    setJniMethodTable("myKill", "(Ljava/lang/String;)V", (void *) clear_activity_stack,
                      &jniNativeMethod[0]);
    int ret = env->RegisterNatives(clazz, jniNativeMethod,
                                   sizeof(jniNativeMethod) / sizeof(jniNativeMethod[0]));
    LOGE("registerNatives ret : %d", ret);
    return JNI_VERSION_1_4;
}
