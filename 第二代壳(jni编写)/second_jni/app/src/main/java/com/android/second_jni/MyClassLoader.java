package com.android.second_jni;

import java.lang.reflect.Method;

import dalvik.system.DexClassLoader;

public class MyClassLoader extends DexClassLoader {
    static {
        System.loadLibrary("usejni");
    }
    public native int openDexFile(byte[] bytes,int len);
    public MyClassLoader(byte bytes[],
                            String dexPath,
                            String optimizedDirectory,
                            String librarySearchPath,
                            ClassLoader parent) {
        super(dexPath, optimizedDirectory, librarySearchPath, parent);

        createDexClassLoader(bytes,parent);

    }
    private ClassLoader mClassLoader;
    private int mCookie;
    private void createDexClassLoader(byte[] bytes, ClassLoader parent) {
        mClassLoader = parent;
        try {
            // Android 4.4 系统
            mCookie = openDexFile(bytes, bytes.length);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    @Override
    public Class<?> loadClass(String name) throws ClassNotFoundException {
        try{
            Class clzDexFile = Class.forName("dalvik.system.DexFile");
            Method method = clzDexFile.getDeclaredMethod("defineClass",
                    String.class,ClassLoader.class,int.class);
            method.setAccessible(true);
            Class clz = (Class)  method.invoke(null,new Object[]{name,mClassLoader,mCookie});
            return clz;
        }catch (Exception e){
            e.printStackTrace();
        }
        return null;
    }
}
