# killAndroidProcessByJNI
kill self process by clear the stack of activity in Android

原理：通过清空activity栈的方式来实现杀掉进程 (也可以在子线程中调用该方法) 

好处：
1.崩溃时看不到堆栈信息, 需要逆向分析该功能
2.应该可以解决在子线程中kill 后, 进程又被 zygote 重复拉起的问题

java 方式：

    public static void processExit(Context context) {
        try {
            Intent intent = null;
            PackageManager packageManager = context.getPackageManager();
            intent = packageManager.getLaunchIntentForPackage(context.getPackageName());
            ComponentName cn = intent.getComponent();
            Intent mainIntent = Intent.makeRestartActivityTask(cn);
            context.startActivity(mainIntent);
            intent = new Intent(Intent.ACTION_MAIN);
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP);
            intent.addCategory(Intent.CATEGORY_HOME);
            context.startActivity(intent);
        } catch (Exception e) {
        } finally {
            System.exit(0);
        }
    }