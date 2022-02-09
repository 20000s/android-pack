package com.android.second_jni;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Intent;
import android.content.res.AssetManager;
import android.os.Bundle;
import android.view.View;
import android.widget.TextView;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.Field;
import java.lang.reflect.Method;

import dalvik.system.DexClassLoader;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);


    }
    public void btnClick(View view){

        byte bytes[] = getdexFromAssets("m2a1.dex");

        MyClassLoader dex = new MyClassLoader(bytes,getPackageCodePath(),
                getCacheDir().toString(),null,getClassLoader());
        Class clz = null;
        try{
            clz = dex.loadClass("com.android.second_jni.MainActivity2");

        }catch (ClassNotFoundException e){
            e.printStackTrace();
        }
        replaceClassLoader1(dex);
        startActivity(new Intent(this,clz));
    }

    byte[] getdexFromAssets(String dexName){
        //获得assets目录管理器
        AssetManager as = getAssets();
        //合成路劲
        String path = getFilesDir() + File.separator + dexName;
        try{
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            //打开文件
            InputStream is = as.open(dexName);
            byte[] buffer = new byte[1024];
            int len = 0;
            while((len = is.read(buffer)) != -1){
                out.write(buffer,0,len);
            }
            return out.toByteArray();
        }catch (IOException e){
            e.printStackTrace();
        }
        return null;
    }
    public void replaceClassLoader1(DexClassLoader dexClassLoader){
        try {
            // 1. 获取ActivityThead类对象
            // android.app.ActivityThread
            // 1.1 获取类类型
            Class clzActivityThead = Class.forName("android.app.ActivityThread");
            // 1.2 获取类方法
            Method currentActivityThread = clzActivityThead.getMethod("currentActivityThread",new Class[]{});
            // 1.3 调用方法
            currentActivityThread.setAccessible(true);
            Object objActivityThread = currentActivityThread.invoke(null);

            // 2. 通过类对象获取成员变量mBoundApplication
            //clzActivityThead.getDeclaredField()
            Field field = clzActivityThead.getDeclaredField("mBoundApplication");
            // AppBindData
            field.setAccessible(true);
            Object data = field.get(objActivityThread);
            // 3. 获取mBoundApplication对象中的成员变量info
            // 3.1 获取 AppBindData 类类型
            Class clzAppBindData = Class.forName("android.app.ActivityThread$AppBindData");
            // 3.2 获取成员变量info
            Field field1 = clzAppBindData.getDeclaredField("info");
            // 3.3 获取对应的值
            //LoadedApk
            field1.setAccessible(true);
            Object info = field1.get(data);
            // 4. 获取info对象中的mClassLoader
            // 4.1 获取 LoadedApk 类型
            Class clzLoadedApk = Class.forName("android.app.LoadedApk");
            // 4.2 获取成员变量 mClassLoader
            Field field2 = clzLoadedApk.getDeclaredField("mClassLoader");
            field2.setAccessible(true);

            // 5. 替换ClassLoader
            field2.set(info,dexClassLoader);

        } catch (Exception e) {
            e.printStackTrace();
        }
    }


}