package com.android.dexshell;

import java.lang.reflect.Field;
import java.lang.reflect.Method;

public class RefInvoke {

    /**
     * invokeStaticMethod
     * @param class_name
     * @param method_name
     * @param pareTyple
     * @param pareVaules
     * @return
     */
    public static  Object invokeStaticMethod(String class_name, String method_name, Class[] pareTyple, Object[] pareVaules){
        try {
            Class obj_class = Class.forName(class_name);
            Method method = obj_class.getMethod(method_name,pareTyple);
            return method.invoke(null, pareVaules);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    /**
     * getFieldOjbect
     * @param class_name
     * @param obj
     * @param filedName
     * @return
     */
    public static Object getFieldOjbect(String class_name,Object obj, String filedName){
        try {
            Class obj_class = Class.forName(class_name);
            Field field = obj_class.getDeclaredField(filedName);
            field.setAccessible(true);
            return field.get(obj);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;
    }

    /**
     * setFieldOjbect
     * @param classname
     * @param filedName
     * @param obj
     * @param filedVaule
     */
    public static void setFieldOjbect(String classname, String filedName, Object obj, Object filedVaule){
        try {
            Class obj_class = Class.forName(classname);
            Field field = obj_class.getDeclaredField(filedName);
            field.setAccessible(true);
            field.set(obj, filedVaule);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * invokeMethod
     * @param class_name
     * @param method_name
     * @param obj
     * @param pareTyple
     * @param pareVaules
     * @return
     */
    public static  Object invokeMethod(String class_name, String method_name, Object obj ,Class[] pareTyple, Object[] pareVaules){

        try {
            Class obj_class = Class.forName(class_name);
            Method method = obj_class.getMethod(method_name,pareTyple);
            return method.invoke(obj, pareVaules);
        } catch (Exception e) {
            e.printStackTrace();
        }
        return null;

    }

}
