//
// Created by 24657 on 2021/4/22.
//
//以下代码只有在davik下 4.1~4.4才能实现
#include "usejni.h"
typedef void (* OPEN_DEX_FILE)(const uint32_t * args,JValue * pResult);
typedef void (* DEFINE_CLASS)(const uint32_t * args,JValue * pResult);
OPEN_DEX_FILE g_openDexFile=NULL;
DEFINE_CLASS g_defineClass = NULL;
//GetFunAddr主要功能是找到函数地址 arg1:函数名称 arg2:函数签名（起始也是smli的返回 参数的简写）
void* GetFunAddr(char * methoidName,char * sig){

    void * handle = dlopen("libdvm.so",RTLD_LAZY);

    LOGD("模块基址为：%p",handle);
    JNINativeMethod  * jniNativeMethod = (JNINativeMethod*)dlsym(handle,"dvm_dalvik_system_DexFile");//这个是个数组。。。
    /*
     * const DalvikNativeMethod dvm_dalvik_system_DexFile[] = {
519    { "openDexFile",        "(Ljava/lang/String;Ljava/lang/String;I)I",
520        Dalvik_dalvik_system_DexFile_openDexFile },
521    { "openDexFile",        "([B)I",
522        Dalvik_dalvik_system_DexFile_openDexFile_bytearray },
523    { "closeDexFile",       "(I)V",
524        Dalvik_dalvik_system_DexFile_closeDexFile },
525    { "defineClass",        "(Ljava/lang/String;Ljava/lang/ClassLoader;I)Ljava/lang/Class;",
526        Dalvik_dalvik_system_DexFile_defineClass },
527    { "getClassNameList",   "(I)[Ljava/lang/String;",
528        Dalvik_dalvik_system_DexFile_getClassNameList },
529    { "isDexOptNeeded",     "(Ljava/lang/String;)Z",
530        Dalvik_dalvik_system_DexFile_isDexOptNeeded },
531    { NULL, NULL, NULL },
     struct DalvikNativeMethod {
29    const char* name;
30    const char* signature;
31    DalvikNativeFunc  fnPtr;
32};
532};
533
     */
    LOGD("数组基址为%p",jniNativeMethod);
    int i = 0;
    JNINativeMethod * nativeMethod = NULL;
    do{
        nativeMethod = (jniNativeMethod + i++);
        LOGD("函数名称为：%s",nativeMethod->name);
        if(strcmp(nativeMethod->name,methoidName)== 0
        && strcmp(nativeMethod->signature,sig)== 0){
            break;
        }
    }while(nativeMethod->name != NULL);

    dlclose(handle);
    return  nativeMethod->fnPtr;
}


extern "C" JNIEXPORT jint JNI_OnLoad(JavaVM * vm,void * reversed){
    JNIEnv env;
    jint jRet = vm->GetEnv((void**) & env,JNI_VERSION_1_6);
    if(jRet != JNI_OK){
        return JNI_ERR;
    }

    g_openDexFile = (OPEN_DEX_FILE)(GetFunAddr("openDexFile","([B)I"));
    return JNI_VERSION_1_6;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_android_second_1jni_MyClassLoader_openDexFile(JNIEnv *env, jobject instance, jbyteArray bytes_,
                                                       jint len) {

    jbyte *bytes = env->GetByteArrayElements(bytes_,NULL);

    ArrayObject *pObject = static_cast<ArrayObject*>(malloc(sizeof(ArrayObject) + len));

    pObject->length = len;
    memcpy(pObject->contents,bytes,len);
    uint32_t  args = {*(uint32_t*)&pObject};
    JValue jRet= {0};
    g_openDexFile(&args,&jRet);

    env->ReleaseByteArrayElements(bytes_,bytes,0);
    //返回cookied
    return jRet.i;
}