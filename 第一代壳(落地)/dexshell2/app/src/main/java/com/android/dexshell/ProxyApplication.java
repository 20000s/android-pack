package com.android.dexshell;

import android.app.Application;
import android.app.Instrumentation;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.util.ArrayMap;
import android.util.Log;

import java.io.BufferedInputStream;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

import dalvik.system.DexClassLoader;

public class ProxyApplication extends Application {
    final String TAG = "APPLICATION_CLASS_NAME";//得到源apk的application
    private String mDexAbsolutePath;//源apk dex的绝对路径
    private String mLibAbsolutePath;//源apk 的so文件
    private String mSrcAokAbsolutePath;//源apk
    //attachbasecontext的用处是替换dexclassloader
    @Override
    protected void attachBaseContext(Context base) {
        super.attachBaseContext(base);

        // 获取存储路径
        File odex = this.getDir("payload_odex",MODE_PRIVATE);
        File libs = this.getDir("payload_lib",MODE_PRIVATE);
        mDexAbsolutePath = odex.getAbsolutePath();//用于存放源apk释放出来的dex
        mLibAbsolutePath = libs.getAbsolutePath();//用于存放源apk用到的so文件
        mSrcAokAbsolutePath = odex.getAbsolutePath() + "/payload.apk";
        File srcApkFile  = new File(mSrcAokAbsolutePath);
        if(!srcApkFile.exists())
        {
            //源apk没有被释放出来（第一次加载）
            try{
                srcApkFile.createNewFile();
            }catch (IOException e){
                e.printStackTrace();
            }
            //创建了文件，将apk放进去
            byte[] shellDexData;
            try{
                shellDexData = getDexFileFromShellApk();//读取classes.dex放在shelldexdata
                releaseSrcApkAndSrcLibFiles(shellDexData);//解密apk 复制libs中的文件在lib下
            } catch (IOException e){
                e.printStackTrace();
            }
        }

        //配置动态加载环境 替换classloader
        Object currentActivityThread = RefInvoke.invokeStaticMethod("android.app.ActivityThread","currentActivityThread",new Class[]{},new Object[]{});
        String packageName = this.getPackageName();
        ArrayMap mPackages = (ArrayMap) RefInvoke.getFieldOjbect("android.app.ActivityThread",currentActivityThread,"mPackages");
        WeakReference weakReference = (WeakReference) mPackages.get(packageName);

        DexClassLoader newDexClassLoader = new DexClassLoader(mSrcAokAbsolutePath,mDexAbsolutePath,mLibAbsolutePath,
                (ClassLoader) RefInvoke.getFieldOjbect("android.app.LoadedApk",weakReference.get(),"mClassLoader"));

         RefInvoke.setFieldOjbect("android.app.LoadedApk","mClassLoader",weakReference.get(),newDexClassLoader);


    }
// oncreate的主要作用是加载apk资源，application 设置activityThread信息
    @Override
    public void onCreate() {
        super.onCreate();

        //源apk启动类
        String srcAppClassName = "";
        //源apk所在路劲
        try{
            ApplicationInfo applicationInfo = this.getPackageManager().getApplicationInfo(this.getPackageName(), PackageManager.GET_META_DATA);
            Bundle bundle = applicationInfo.metaData;
            if(bundle != null && bundle.containsKey(TAG)){
                srcAppClassName = bundle.getString(TAG);

            }else {
                Log.d("demo","can not find the information of application");
                return;
            }

        }
        catch (Exception e)
        {
            Log.d("demo","不能找到包管理器");

        }
        //获取activitythread类下的信息 ，然后替换
        Object currentActivityThread = RefInvoke.invokeStaticMethod("android.app.ActivityThread","currentActivityThread"
        ,new Class[]{},new Object[]{});
        Object mBoundApplication = RefInvoke.getFieldOjbect("android.app.ActivityThread",currentActivityThread,
                "mBoundApplication");
        Object loadedApkInfo = RefInvoke.getFieldOjbect("android.app.ActivityThread$AppBindData",mBoundApplication
        ,"info");
        //将原来的loadedApkinfo mapplication置空
        RefInvoke.setFieldOjbect("android.app.LoadedApk","mApplication",loadedApkInfo,null);
        //获取壳线程的application
        Object oldApplication = RefInvoke.getFieldOjbect("android.app.ActivityThread",currentActivityThread,
                "mInitialApplication");
        ArrayList<Application> mAllApplications = (ArrayList<Application>) RefInvoke.getFieldOjbect(
                "android.app.ActivityThread", currentActivityThread, "mAllApplications"
        );
        //移除原来的application
        mAllApplications.remove(oldApplication);
        //构造新的application
        ApplicationInfo appinfo_In_LoadedApk = (ApplicationInfo) RefInvoke.getFieldOjbect("android.app.LoadedApk", loadedApkInfo, "mApplicationInfo");
        ApplicationInfo appinfo_In_AppBindData = (ApplicationInfo) RefInvoke.getFieldOjbect("android.app.ActivityThread$AppBindData", mBoundApplication, "appInfo");
        appinfo_In_LoadedApk.className = srcAppClassName;
        appinfo_In_AppBindData.className = srcAppClassName;
        //注册application
        Application app = (Application) RefInvoke.invokeMethod(
                "android.app.LoadedApk","makeApplication",loadedApkInfo,
                new Class[]{boolean.class, Instrumentation.class}, new Object[]{false,null}
        );
       //替换activityThread中的mInitialApplication
        RefInvoke.setFieldOjbect("android.app.ActivityThread", "mInitialApplication", currentActivityThread, app);
       //替换之前的内容提供者为刚刚注册的app
        ArrayMap mProviderMap = (ArrayMap) RefInvoke.getFieldOjbect("android.app.ActivityThread", currentActivityThread, "mProviderMap");

        Iterator it = mProviderMap.values().iterator();
        while (it.hasNext()) {
            Object providerClientRecord = it.next();
            Object localProvider = RefInvoke.getFieldOjbect("android.app.ActivityThread$ProviderClientRecord", providerClientRecord, "mLocalProvider");
            RefInvoke.setFieldOjbect("android.content.ContentProvider", "mContext", localProvider, app);
        }

        app.onCreate();

    }
    private byte[] getDexFileFromShellApk() throws IOException{
        ByteArrayOutputStream dexByteArrayOutputStream = new ByteArrayOutputStream();
        ZipInputStream localZipInputStream = new ZipInputStream(new BufferedInputStream(new FileInputStream(this.getApplicationInfo().sourceDir)));
        while (true){
            ZipEntry localZipEntry = localZipInputStream.getNextEntry();
            if(localZipEntry == null)
            {
                localZipInputStream.close();
                break;
            }
            if(localZipEntry.getName().equals("classes.dex"))
            {
                byte[] arrayOfByte = new byte[1024];
                while (true)
                {
                    int i = localZipInputStream.read(arrayOfByte);
                    if(i== -1)
                        break;
                    dexByteArrayOutputStream.write(arrayOfByte,0,i);

                }
            }
            localZipInputStream.closeEntry();
        }
        localZipInputStream.close();
        return  dexByteArrayOutputStream.toByteArray();
    }
    private void releaseSrcApkAndSrcLibFiles(byte[] shellDexData) throws IOException{
        int shellDexDataLength = shellDexData.length;
        //取被加壳apk长度（长度存储在壳dex的最后四个字节）
        byte[] dexlen = new byte[4];
        System.arraycopy(shellDexData,shellDexDataLength-4,dexlen,0,4);
        ByteArrayInputStream byteArrayInputStream = new ByteArrayInputStream(dexlen);
        DataInputStream dataInputStream = new DataInputStream(byteArrayInputStream);
        int srcDexSize = dataInputStream.readInt();
        //取出apk
        byte[] encryptedSrcApkData = new byte[srcDexSize];
        System.arraycopy(shellDexData,shellDexDataLength-4-srcDexSize,encryptedSrcApkData,0,srcDexSize);
        //对源程序解密
        byte[] decryptedArcAokData = decryptSrcApk(encryptedSrcApkData);

        //写入源apk文件
        File file = new File(mSrcAokAbsolutePath);
        try{
            FileOutputStream localFileOutputStream = new FileOutputStream(file);
            localFileOutputStream.write(decryptedArcAokData);
            localFileOutputStream.close();
        } catch (IOException e){
            throw  new RuntimeException(e);
        }
        //分析源apk
        ZipInputStream localZipInputStream = new ZipInputStream(new BufferedInputStream(new FileInputStream(file)));
        while (true)
        {
            ZipEntry srcApk = localZipInputStream.getNextEntry();
            if(srcApk == null){
                localZipInputStream.close();
                break;
            }
            //依次取出被加壳apk用到的so文件，放到 libPath中（data/data/包名/payload_lib)
            String fileName = srcApk.getName();
            if (fileName.startsWith("lib/") && fileName.endsWith(".so"))
            {
                File fileInSrcLibDir = new File(mLibAbsolutePath + "/" + fileName.substring(fileName.lastIndexOf('/')));
                fileInSrcLibDir.createNewFile();
                FileOutputStream fileOutputStream = new FileOutputStream(fileInSrcLibDir);
                // 复制文件到当前壳程序的lib目录下
                byte[] copyBuffer = new byte[1024];
                while (true)
                {
                    int i = localZipInputStream.read(copyBuffer);
                    if (i == -1)
                        break;
                    fileOutputStream.write(copyBuffer, 0, i);
                }
                fileOutputStream.flush();
                fileOutputStream.close();
            }
            localZipInputStream.closeEntry();
        }
        localZipInputStream.close();

        }
        private byte[] decryptSrcApk(byte[] srcApkData)
        {
            for(int i = 0 ; i < srcApkData.length ; ++i)
            {
                srcApkData[i] = (byte)(0xff ^ srcApkData[i]);
            }
            return srcApkData;
        }


}
