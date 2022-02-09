//
// Created by 24657 on 2021/4/29.
//

#include "loaddex.h"
#include <jni.h>
#include <dlfcn.h>
#include <android/log.h>
#include <errno.h>
#include <assert.h>
#include <sys/system_properties.h>
#include <string>
#include <cstdlib>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <vector>
#include <memory.h>
#include <locale>
#include "common.h"
#include "dex_header.h"
#include "utils.h"
#include "byte_load.h"
#include "hook_instance.h"
#include "elfGotHook/tools.h"
#include "elfGotHook/elf_reader.h"

#define CBC 1
#define CTR 1
#define ECB 1
#include "aes.hpp"

// #define PAGE_MASK 0xfffff000
// #define PAGE_START(x) ((x)&PAGE_MASK)
#define PAGE_SIZE 4096
#define PAGE_OFFSET(x) ((x) & ~PAGE_MASK)
#define PAGE_END(x) PAGE_START((x) + (PAGE_SIZE - 1))

#if defined(__aarch64__)
#define LIB_ART_PATH "/system/lib64/libart.so"
#elif defined(__arm__)
#define LIB_ART_PATH "/system/lib/libart.so"
#else
#define LIB_ART_PATH "/system/lib/libart.so"
#endif

#define REGREX ".*/libart\\.so$"
#define JIAMI_MAGIC "jiami.dat"
#define PACKER_MAGIC ".jiagu"

using namespace std;

char g_jiagu_dir[256] = {0}; //加密dex所在目录
bool g_isArt = false;
int g_sdk_int = 0;
const char * g_file_dir;
const char * g_NativeLibDir;
char * g_PackageResourcePath;
char *g_pkgName;
int g_dex_size = 0;  //main.dex的真正大小
int g_page_size = 0; //main.dex进行页面对齐后的大小
char g_fake_dex_magic[256] = {0};
void * g_decrypt_base = NULL;
void * g_ArtHandle = NULL;

const char *AES_KEYCODE = "1234567812345678";
const char *AES_IV = "1234567812345678";

void native_attachBaseContext(JNIEnv *env, jobject obj, jobject ctx);
void native_onCreate(JNIEnv *, jobject, jobject);

unsigned char MINIDEX[292] = {
        0x64, 0x65, 0x78, 0x0A, 0x30, 0x33, 0x35, 0x00, 0xD9, 0x24, 0x14, 0xFD, 0x2F, 0x81, 0x4D, 0x8B,
        0x50, 0x48, 0x13, 0x1D, 0x8D, 0xA9, 0xCF, 0x1F, 0xF1, 0xF2, 0xDD, 0x06, 0xB4, 0x67, 0x70, 0xA1,
        0x24, 0x01, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x78, 0x56, 0x34, 0x12, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xD8, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00,
        0x02, 0x00, 0x00, 0x00, 0x7C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x84, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0xA4, 0x00, 0x00, 0x00,
        0xA4, 0x00, 0x00, 0x00, 0xB4, 0x00, 0x00, 0x00, 0xC8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x0E, 0x4C, 0x63, 0x6F, 0x6D, 0x2F, 0x6D, 0x69, 0x78, 0x43, 0x6C, 0x61,
        0x73, 0x73, 0x3B, 0x00, 0x12, 0x4C, 0x6A, 0x61, 0x76, 0x61, 0x2F, 0x6C, 0x61, 0x6E, 0x67, 0x2F,
        0x4F, 0x62, 0x6A, 0x65, 0x63, 0x74, 0x3B, 0x00, 0x0D, 0x6D, 0x69, 0x78, 0x43, 0x6C, 0x61, 0x73,
        0x73, 0x2E, 0x6A, 0x61, 0x76, 0x61, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
        0x70, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x7C, 0x00, 0x00, 0x00,
        0x06, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x84, 0x00, 0x00, 0x00, 0x02, 0x20, 0x00, 0x00,
        0x03, 0x00, 0x00, 0x00, 0xA4, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0xD8, 0x00, 0x00, 0x00};

void write_mix_dex(const char * minidex)
{
    if(access(minidex,F_OK) == -1)
    {
        FILE  * file = fopen(minidex,"wb");
        fwrite(MINIDEX,292,1,file);
        fclose(file);
        LOGD("[+]mix dex saved at:%s",minidex);
    }
}

static JNINativeMethod methods[] = {
        {"attachBaseContext", "(Landroid/content/Context;)V", (void *)native_attachBaseContext},
        {"onCreate", "(Landroid/content/Context;)V", (void *)native_onCreate}};

int jniRegisterNativeMethods(JNIEnv *env, const char *className, const JNINativeMethod *gMethods, int numMethods)
{
    jclass clazz;
    int tmp;

    clazz = env->FindClass(className);
    if(clazz == NULL)
    {
        return -1;
    }
    if((tmp = env->RegisterNatives(clazz,gMethods,numMethods)) < 0)
    {
        LOGE("[[-] RegisterNatives failed");
        return -1;
    }
    return 0;

}

char * parse_file(const char * encrypt_path,int & encrypt_size)
{
    FILE * fp = fopen(encrypt_path,"rb+");

    if(!fp)
    {
        LOGE("[-]fopen %s error: %s",encrypt_path,strerror(errno));
        return NULL;
    }
    fseek(fp,0L,SEEK_END);
    encrypt_size = ftell(fp);
    fseek(fp,0L,SEEK_SET);
    char *base = (char *)calloc(encrypt_size + 1,sizeof(char));
    if(!base)
    {
        LOGE("calloc memory failed");
        fclose(fp);
        return NULL;
    }
    fread(base, 1, encrypt_size, fp);
    fclose(fp);
    return base;
}
char *tiny_aes_encrypt_cbc(char *in, int inLen, int *outLen)
{

#define AES_BLOCK_SIZE 16
    char *inputData = NULL;
    unsigned char Key[AES_BLOCK_SIZE + 1];
    unsigned char ivec[AES_BLOCK_SIZE];

    //设置密钥key
    memset(Key, 0x00, sizeof(Key));
    memcpy(Key, AES_KEYCODE, AES_BLOCK_SIZE);

    //设置ivec
    memcpy(ivec, AES_IV, 16);

    int nBei = inLen / AES_BLOCK_SIZE + 1;
    int nTotal = nBei * AES_BLOCK_SIZE;

    *outLen = nTotal;

    //使用PKCS5Padding
    inputData = (char *)calloc(nTotal + 1, 1);
    int nNumber;
    if (inLen % 16)
    {
        nNumber = nTotal - inLen;
    }
    else
    {
        nNumber = 16;
    }
    memset(inputData, nNumber, nTotal);
    memcpy(inputData, in, inLen);

    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, Key, ivec);
    AES_CBC_encrypt_buffer(&ctx, (uint8_t *)inputData, (uint32_t)nTotal);

    return inputData;
}

char *tiny_aes_decrypt_cbc(char *in, int inLen, int *outLen)
{

#define AES_BLOCK_SIZE 16
    unsigned char Key[AES_BLOCK_SIZE + 1];
    unsigned char ivec[AES_BLOCK_SIZE];

    //设置密钥key
    memset(Key, 0x00, sizeof(Key));
    memcpy(Key, AES_KEYCODE, 16);

    memcpy(ivec, AES_IV, 16);

    struct AES_ctx ctx;
    AES_init_ctx_iv(&ctx, Key, ivec);

    AES_CBC_decrypt_buffer(&ctx, (uint8_t *)in, (uint32_t)inLen);

    //去掉padding字符串
    //获取padding后的明文长度
    int padLen = inLen;
    //获取pad的值
    int padValue = in[padLen - 1];
    LOGD("[+]padValue:%d", padValue);

    if ((padValue == 0) || (padValue > AES_BLOCK_SIZE))
    {
        //错误的padding，解密失败
        LOGE("[-]decrypt failed");
        return 0;
    }
    int realLen = padLen - padValue;
    *outLen = realLen;
    in[padLen - padValue] = '\0';
    // char saveFile[0x100] = {0};
    // sprintf(saveFile, "%s/dump.dex", g_jiagu_dir);
    // FILE *file = fopen(saveFile, "wb");
    // fwrite(in, 1, realLen, file);
    // fclose(file);
    // LOGD("[+]dump.dex saved at %s,size:%d", saveFile, realLen);
    return in;
}
void * openmemory_load_dex(void * art_handle,char *base,size_t size,int sdk_int)
{
    void *c_dex_cookie;
    switch (sdk_int)
    {
        //android 4.4 art mode
        case 19:
            c_dex_cookie = mem_loadDex_byte19(art_handle, base, g_dex_size);
            break;

        case 21:
            c_dex_cookie = mem_loadDex_byte21(art_handle, base, g_dex_size);
            break;

        case 22:
            c_dex_cookie = mem_loadDex_byte22(art_handle, base, g_dex_size);
            break;

        case 23:
            c_dex_cookie = mem_loadDex_byte23(art_handle, base, g_dex_size);
            break;

            // 7.0 and 7.1
        case 24:
        case 25:
            //c_dex_cookie = mem_loadDex_byte24(g_ArtHandle, (char *)g_decrypt_base, (size_t)g_dex_size);
            c_dex_cookie = NULL;
            break;

            //8.0
        case 26:
        case 27:
            //reserved
            c_dex_cookie = NULL;
            break;

        default:
            c_dex_cookie = NULL;
            break;
    }
    return c_dex_cookie;
}

void *get_lib_handle(const char *lib_path)
{
    void *handle_art = dlopen(lib_path, RTLD_NOW);

    if (!handle_art)
    {
        LOGE("[-]get %s handle failed:%s", lib_path, dlerror());
        return NULL;
    }
    return handle_art;
}
jobject load_dex_fromfile(JNIEnv *env, const char *inPath, const char *outPath)
{
    jclass DexFileClass = env->FindClass("dalvik/system/DexFile"); // "dalvik/system/DexPathList$Element"
    // new DexFile==loadDex
    // loadDex方法比<init>方法通用性更好
    // jmethodID init = env->GetMethodID(DexFileClass, "<init>", "(Ljava/lang/String;Ljava/lang/String;I)V");
    jmethodID init = env->GetStaticMethodID(DexFileClass, "loadDex",
                                            "(Ljava/lang/String;Ljava/lang/String;I)Ldalvik/system/DexFile;");

    if (env->ExceptionCheck())
    {
        LOGE("[-]get loadDex methodID  error");
        return 0;
    }
    jstring apk = env->NewStringUTF(inPath);
    jstring odex = env->NewStringUTF(outPath);

    jobject dexobj = env->CallStaticObjectMethod(DexFileClass, init, apk, odex, 0);
    if (env->ExceptionCheck())
    {
        LOGE("[-]loadDex %s dex failed", inPath);
        return 0;
    }

    env->DeleteLocalRef(DexFileClass);
    env->DeleteLocalRef(apk);
    env->DeleteLocalRef(odex);

    // try delete mixdexobj localRef
    // env->DeleteLocalRef(mixdexobj);
    return dexobj;
}
void make_dex_elements(JNIEnv *env, jobject classLoader, jobject dexFileobj)
{
    jclass PathClassLoader = env->GetObjectClass(classLoader);
    jclass BaseDexClassLoader = env->GetSuperclass(PathClassLoader);
    // get pathList fieldid
    jfieldID pathListid = env->GetFieldID(BaseDexClassLoader, "pathList", "Ldalvik/system/DexPathList;");
    jobject pathList = env->GetObjectField(classLoader, pathListid);

    // get DexPathList Class
    jclass DexPathListClass = env->GetObjectClass(pathList);
    // get dexElements fieldid
    jfieldID dexElementsid = env->GetFieldID(DexPathListClass, "dexElements", "[Ldalvik/system/DexPathList$Element;");
    jobjectArray dexElement = static_cast<jobjectArray>(env->GetObjectField(pathList, dexElementsid));

    jint len = env->GetArrayLength(dexElement);

    LOGD("[+]Elements size:%d", len);

    jclass ElementClass = env->FindClass("dalvik/system/DexPathList$Element"); // dalvik/system/DexPathList$Element
    jmethodID Elementinit = env->GetMethodID(ElementClass, "<init>",
                                             "(Ljava/io/File;ZLjava/io/File;Ldalvik/system/DexFile;)V");
    jboolean isDirectory = JNI_FALSE;

    jobject element_obj = env->NewObject(ElementClass, Elementinit, NULL, isDirectory, NULL, dexFileobj);

    // Get dexElement all values and add  add each value to the new array
    jobjectArray new_dexElement = env->NewObjectArray(len + 1, ElementClass, NULL);
    for (int i = 0; i < len; ++i)
    {
        env->SetObjectArrayElement(new_dexElement, i, env->GetObjectArrayElement(dexElement, i));
    }

    env->SetObjectArrayElement(new_dexElement, len, element_obj);
    env->SetObjectField(pathList, dexElementsid, new_dexElement);

    env->DeleteLocalRef(element_obj);
    env->DeleteLocalRef(ElementClass);
    env->DeleteLocalRef(dexElement);
    env->DeleteLocalRef(DexPathListClass);
    env->DeleteLocalRef(pathList);
    env->DeleteLocalRef(BaseDexClassLoader);
    env->DeleteLocalRef(PathClassLoader);
}
jobject hook_load_dex_internally(JNIEnv *env, const char *art_path, char *inPath, char *outPath)
{
    void *art_base = get_module_base(getpid(), art_path);
    if (!art_base)
    {
        LOGE("[-]get lib %s base failed", art_path);
        return NULL;
    }
    ElfReader elfReader(art_path, art_base);
    if (0 != elfReader.parse())
    {
        LOGE("failed to parse %s in %d maps at %p", LIB_ART_PATH, getpid(), art_base);
        return NULL;
    }
    elfReader.hook("open", (void *)new_open, (void **)&old_open);
    elfReader.hook("read", (void *)new_read, (void **)&old_read);
    elfReader.hook("mmap", (void *)new_mmap, (void **)&old_mmap);
    elfReader.hook("munmap", (void *)new_munmap, (void **)&old_munmap);
    elfReader.hook("__read_chk", (void *)new_read_chk, (void **)&old_read_chk);
    elfReader.hook("fstat", (void *)new_fstat, (void **)&old_fstat);
    elfReader.hook("fork", (void *)new_fork, (void **)&old_fork);
    LOGD("[+]Load fake dex inPath:%s,outPath:%s", inPath, outPath);
    jobject faked_dex_obj = load_dex_fromfile(env, inPath, outPath);
    LOGD("[+]Load fake dex finished");
    //恢复fork和fstat的hook
    elfReader.hook("fork", (void *)old_fork, (void **)&old_fork);
    elfReader.hook("fstat", (void *)old_fstat, (void **)&old_fstat);
    return faked_dex_obj;
}

void mem_loadDex(JNIEnv * env,jobject ctx,const char * dex_path)
{

    char inPath[256] = {0};
    char outPath[256] = {0};
    jobject  mini_dex_obj = NULL;
    void * c_dex_cookie = NULL;

    jclass ApplicationClass = env->GetObjectClass(ctx);
    jmethodID getClassLoader = env->GetMethodID(ApplicationClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
    jobject classLoader = env->CallObjectMethod(ctx,getClassLoader);

    char szDexPath[256] = {0};

    sprintf((char*)szDexPath,dex_path,strlen(dex_path));
    LOGD("[+]Dex Path : %s",szDexPath);

    //读取加密dex
    int encrypt_size;
    char * encrypt_buffer = parse_file(szDexPath,encrypt_size);
    //check dex size
    if(!encrypt_size)
    {
        LOGE("[-]encrypt_size is 0");
        exit(-1);
    }else if(encrypt_size % 16)
    {
        LOGE("[-]encrypt_size is not mutiple 16");
        exit(-1);
    }

    int zero = open("/dev/zero",PROT_WRITE);
    g_decrypt_base = mmap(0, encrypt_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, zero, 0);
    close(zero);
    if (g_decrypt_base == MAP_FAILED)
    {
        LOGE("[-]ANONYMOUS mmap failed:%s", strerror(errno));
        exit(-1);
    }
    char decrypt_path[256] = {0};
    sprintf((char *)decrypt_path, "%s/decrypt.dat", g_jiagu_dir);

    char *decrypt_buffer = tiny_aes_decrypt_cbc(encrypt_buffer, encrypt_size, &g_dex_size);
    if (!decrypt_buffer)
    {
        LOGE("[-]aes_decrypt_cbc decrypt dex failed");
        exit(-1);
    }
    memcpy(g_decrypt_base,decrypt_buffer,g_dex_size);
    g_page_size = PAGE_END(g_dex_size);
    free(decrypt_buffer);
    LOGD("[+]After decrypt dex magic:0x%x,size:%d,page_size:%d", *(int *)g_decrypt_base, g_dex_size, g_page_size);

    if(g_isArt)
    {
        sprintf(inPath, "%s/mini.dex", g_jiagu_dir);
        sprintf(outPath, "%s/mini.oat", g_jiagu_dir);
        write_mix_dex(inPath);
        g_ArtHandle = get_lib_handle(LIB_ART_PATH);
        if(g_ArtHandle)
        {
            c_dex_cookie = openmemory_load_dex(g_ArtHandle,(char*)g_decrypt_base,(size_t)g_dex_size,g_sdk_int);
            if(!c_dex_cookie)
            {
                LOGD("[-]try second plan");
                sprintf((char*)g_fake_dex_magic,"%s/mini.dex",PACKER_MAGIC);
                LOGD("[+]g_faked_dex_magic:%s",(char*)g_fake_dex_magic);
                // 加载fake_dex
                mini_dex_obj = hook_load_dex_internally(env, (const char *)LIB_ART_PATH, (char *)inPath, outPath);

                make_dex_elements(env, classLoader, mini_dex_obj);
            }
            if (g_ArtHandle)
            {
                dlclose(g_ArtHandle);
            }
            return;
        }
    }
}
void native_onCreate(JNIEnv *env, jobject thiz, jobject instance)
{
    LOGD("[+]native onCreate is called");
    jclass ProxyApplicationClass = env->GetObjectClass(instance);
    jmethodID getPackageManager = env->GetMethodID(ProxyApplicationClass, "getPackageManager", "()Landroid/content/pm/PackageManager;");
    if (env->ExceptionCheck())
    {
        LOGE("[-]find getPackageManager methodID failed");
        return;
    }
    jobject packageManager = env->CallObjectMethod(instance, getPackageManager);
    if (env->ExceptionCheck())
    {
        LOGE("[-]call getPackageManager method failed");
        return;
    }
    jmethodID pmGetApplicationInfo = env->GetMethodID(env->GetObjectClass(packageManager), "getApplicationInfo", "(Ljava/lang/String;I)Landroid/content/pm/ApplicationInfo;");
    jmethodID getPackageName = env->GetMethodID(ProxyApplicationClass, "getPackageName", "()Ljava/lang/String;");
    jobject pmAppInfo = env->CallObjectMethod(packageManager, pmGetApplicationInfo, env->CallObjectMethod(instance, getPackageName), 128);

    jclass PackageItemInfoClass = env->FindClass("android/content/pm/PackageItemInfo");

    jfieldID metaDataField = env->GetFieldID(PackageItemInfoClass, "metaData", "Landroid/os/Bundle;");
    jobject metaData = env->GetObjectField(pmAppInfo, metaDataField);
    if (metaData == NULL)
    {
        LOGE("[-]not found meta Bundle");
        return;
    }

    jmethodID bundleGetString = env->GetMethodID(env->GetObjectClass(metaData), "getString", "(Ljava/lang/String;)Ljava/lang/String;");
    //found orignal ApplicationName
    jstring originApplicationName = (jstring)env->CallObjectMethod(metaData, bundleGetString, env->NewStringUTF("APP_NAME"));
    if (originApplicationName == NULL)
    {
        LOGE("[-]not found original Application Name");
        return;
    }
    LOGD("[+]original Application Name : %s", env->GetStringUTFChars(originApplicationName, NULL));

    //将LoadedApk中的mApplication对象替换
    jclass ActivityThreadClass = env->FindClass("android/app/ActivityThread");
    jmethodID currentActivityThread = env->GetStaticMethodID(ActivityThreadClass, "currentActivityThread", "()Landroid/app/ActivityThread;");
    jobject activityThread = env->CallStaticObjectMethod(ActivityThreadClass, currentActivityThread);
    LOGE("get ActivityThreadClass");
    //得到AppBindData
    jfieldID mBoundApplicationField = env->GetFieldID(ActivityThreadClass, "mBoundApplication", "Landroid/app/ActivityThread$AppBindData;");
    jobject mBoundApplication = env->GetObjectField(activityThread, mBoundApplicationField);
    LOGE("get AppBindData");
    //得到LoadedApk
    jfieldID infoField = env->GetFieldID(env->GetObjectClass(mBoundApplication), "info", "Landroid/app/LoadedApk;");
    jobject info = env->GetObjectField(mBoundApplication, infoField);
    LOGE("get LoadedApk");
    //把LoadedApk中的成员变量private Application mApplication;置空
    jfieldID mApplicationField = env->GetFieldID(env->GetObjectClass(info), "mApplication", "Landroid/app/Application;");
    env->SetObjectField(info, mApplicationField, NULL);
    LOGE("mApplication set null");
    //得到壳Application
    jfieldID mInitialApplicationField = env->GetFieldID(ActivityThreadClass, "mInitialApplication", "Landroid/app/Application;");
    jobject mInitialApplication = env->GetObjectField(activityThread, mInitialApplicationField);
    LOGE("get packer Application");

    //将壳Application移除
    jfieldID mAllApplicationsField = env->GetFieldID(ActivityThreadClass, "mAllApplications", "Ljava/util/ArrayList;");
    jobject mAllApplications = env->GetObjectField(activityThread, mAllApplicationsField);
    jmethodID remove = env->GetMethodID(env->GetObjectClass(mAllApplications), "remove", "(Ljava/lang/Object;)Z");
    env->CallBooleanMethod(mAllApplications, remove, mInitialApplication);
    LOGE("remove packer Application");
    //得到AppBindData中的ApplicationInfo
    jfieldID appInfoField = env->GetFieldID(env->GetObjectClass(mBoundApplication), "appInfo", "Landroid/content/pm/ApplicationInfo;");
    jobject appInfo = env->GetObjectField(mBoundApplication, appInfoField);
    LOGE("get AppBindData's ApplicationInfo");
    //得到LoadedApk中的ApplicationInfo
    jfieldID mApplicationInfoField = env->GetFieldID(env->GetObjectClass(info), "mApplicationInfo", "Landroid/content/pm/ApplicationInfo;");
    jobject mApplicationInfo = env->GetObjectField(info, mApplicationInfoField);
    LOGE("get LoadedApk's ApplicationInfo");
    //替换掉ApplicationInfo中的className
    jfieldID classNameField = env->GetFieldID(env->GetObjectClass(appInfo), "className", "Ljava/lang/String;");
    env->SetObjectField(appInfo, classNameField, originApplicationName);
    env->SetObjectField(mApplicationInfo, classNameField, originApplicationName);
    LOGE("replace ApplicationInfo's className");
    //创建新的Application
    jmethodID makeApplication = env->GetMethodID(env->GetObjectClass(info), "makeApplication", "(ZLandroid/app/Instrumentation;)Landroid/app/Application;");
    //这里调用原始app的attacheBaseContext
    jobject originApp = env->CallObjectMethod(info, makeApplication, false, NULL);
    LOGE("create new Application");
    //将句柄赋值到mInitialApplicationField
    env->SetObjectField(activityThread, mInitialApplicationField, originApp);
    LOGE("set object mInitialApplicationField");
    jfieldID mProviderMapField;
    if (g_sdk_int < 19)
    {
        mProviderMapField = env->GetFieldID(ActivityThreadClass, "mProviderMap", "Ljava/util/HashMap;");
    }
    else
    {
        mProviderMapField = env->GetFieldID(ActivityThreadClass, "mProviderMap", "Landroid/util/ArrayMap;");
    }
    if (mProviderMapField == NULL)
    {
        LOGE("not found mProviderMapField");
        return;
    }
    LOGE("found mProviderMapField");
    jobject mProviderMap = env->GetObjectField(activityThread, mProviderMapField);
    LOGE("found mProviderMap");
    jmethodID values = env->GetMethodID(env->GetObjectClass(mProviderMap), "values", "()Ljava/util/Collection;");
    jobject collections = env->CallObjectMethod(mProviderMap, values);
    jmethodID iterator = env->GetMethodID(env->GetObjectClass(collections), "iterator", "()Ljava/util/Iterator;");
    jobject mIterator = env->CallObjectMethod(collections, iterator);
    jmethodID hasNext = env->GetMethodID(env->GetObjectClass(mIterator), "hasNext", "()Z");
    jmethodID next = env->GetMethodID(env->GetObjectClass(mIterator), "next", "()Ljava/lang/Object;");

    //替换所有ContentProvider中的Context
    LOGE("ready replace all ContentProvider's context");
    while (env->CallBooleanMethod(mIterator, hasNext))
    {
        jobject providerClientRecord = env->CallObjectMethod(mIterator, next);
        if (providerClientRecord == NULL)
        {
            LOGE("providerClientRecord = NULL");
            continue;
        }
        jclass ProviderClientRecordClass = env->FindClass("android/app/ActivityThread$ProviderClientRecord");
        jfieldID mLocalProviderField = env->GetFieldID(ProviderClientRecordClass, "mLocalProvider", "Landroid/content/ContentProvider;");
        if (mLocalProviderField == NULL)
        {
            LOGE("mLocalProviderField not found");
            continue;
        }
        jobject mLocalProvider = env->GetObjectField(providerClientRecord, mLocalProviderField);
        if (mLocalProvider == NULL)
        {
            LOGE("mLocalProvider is NULL");
            continue;
        }
        jfieldID mContextField = env->GetFieldID(env->GetObjectClass(mLocalProvider), "mContext", "Landroid/content/Context;");
        if (mContextField == NULL)
        {
            LOGE("mContextField not found");
            continue;
        }
        env->SetObjectField(mLocalProvider, mContextField, originApp);
    }

    //执行originApp的onCreate
    jmethodID onCreate = env->GetMethodID(env->GetObjectClass(originApp), "onCreate", "()V");
    env->CallVoidMethod(originApp, onCreate);
    LOGD("Packer is done");
}
void native_attachBaseContext(JNIEnv * env,jobject thiz,jobject ctx)
{
#if defined(__arm__)
    LOGD("[+]Running arm libdexload");
#elif defined(__aarch64__)
    LOGD("[+]Running aarch64 libdexload");

#endif
      jclass ApplicationClass = env->GetObjectClass(ctx);
      jmethodID getFilesDir = env->GetMethodID(ApplicationClass,"getFilesDir","()Ljava/io/File;");
      jobject File_obj = env->CallObjectMethod(ctx,getFilesDir);
      jclass FileClass = env->GetObjectClass(File_obj);

      jmethodID getAbsolutePath = env->GetMethodID(FileClass,"getAbsolutePath", "()Ljava/lang/String;");
    jstring data_file_dir = static_cast<jstring>(env->CallObjectMethod(File_obj, getAbsolutePath));

    g_file_dir = env->GetStringUTFChars(data_file_dir, NULL);
    //g_file_dir_backup=g_file_dir;
    LOGD("[+]FilesDir:%s", g_file_dir);
    env->DeleteLocalRef(data_file_dir);
    env->DeleteLocalRef(File_obj);
    env->DeleteLocalRef(FileClass);

    jmethodID getApplicationInfo = env->GetMethodID(ApplicationClass,"getApplicationInfo","()Landroid/content/pm/ApplicationInfo;");
    jobject ApplicationInfo_obj = env->CallObjectMethod(ctx,getApplicationInfo);
    jclass ApplicationInfoClass = env->GetObjectClass(ApplicationInfo_obj);
    jfieldID nativeLibraryDir_field = env->GetFieldID(ApplicationInfoClass,"nativeLibraryDir","Ljava/lang/String;");
    jstring nativeLibraryDir = (jstring)(env->GetObjectField(ApplicationInfo_obj,nativeLibraryDir_field));

    g_NativeLibDir = env->GetStringUTFChars(nativeLibraryDir,NULL);
    LOGD("[+]NativeLibDir: %s",g_NativeLibDir);

    env->DeleteLocalRef(nativeLibraryDir);
    env->DeleteLocalRef(ApplicationInfoClass);
    env->DeleteLocalRef(ApplicationInfo_obj);

    jmethodID getPackageResourcePath = env->GetMethodID(ApplicationClass, "getPackageResourcePath",
                                                        "()Ljava/lang/String;");

    jstring mPackageFilePath = static_cast<jstring>(env->CallObjectMethod(ctx, getPackageResourcePath));
    const char *cmPackageFilePath = env->GetStringUTFChars(mPackageFilePath, NULL);
    g_PackageResourcePath = const_cast<char *>(cmPackageFilePath);
    LOGD("[+]PackageResourcePath:%s", g_PackageResourcePath);
    env->DeleteLocalRef(mPackageFilePath);

    jmethodID getPackageName = env->GetMethodID(ApplicationClass, "getPackageName", "()Ljava/lang/String;");
    jstring PackageName = static_cast<jstring>(env->CallObjectMethod(ctx, getPackageName));
    const char *packagename = env->GetStringUTFChars(PackageName, NULL);
    g_pkgName = (char *)packagename;
    LOGD("[+]g_pkgName :%s", g_pkgName);
    env->DeleteLocalRef(PackageName);

    char jiaguPath[256] = {0}; // 加密dex的存储路径

    sprintf(g_jiagu_dir, "%s/%s", g_file_dir, PACKER_MAGIC);
    sprintf(jiaguPath, "%s/%s", g_jiagu_dir, JIAMI_MAGIC);
    LOGD("[+]g_jiagu_dir:%s,jiaguPath:%s", g_jiagu_dir, jiaguPath);
    if (access(g_jiagu_dir, F_OK) != 0)
    {
        if (mkdir(g_jiagu_dir, 0755) == -1)
        {
            LOGE("[-]mkdir %s error:%s", g_jiagu_dir, strerror(errno));
        }
    }
    //从assets目录提取加密dex
    extract_file(env, ctx, jiaguPath, JIAMI_MAGIC);
    mem_loadDex(env, ctx, jiaguPath);

}

void init(JNIEnv *env) {//查看vm版本是不是art,然后动态注册
    jclass jclazz = env->FindClass("android/os/Build$VERSION");
    jfieldID SDK_INT = env->GetStaticFieldID(jclazz,"SDK_INT","I");

    g_sdk_int = env->GetStaticIntField(jclazz,SDK_INT);

    LOGD("[+]sdk_int : %d",g_sdk_int);
    if(g_sdk_int > 13)
    {
        jclass System = env->FindClass("java/lang/System");
        jmethodID System_getProperty = env->GetStaticMethodID(System,"getProperty", "(Ljava/lang/String;)Ljava/lang/String;");

        jstring vm_version_name = env->NewStringUTF("java.vm.version");
        jstring vm_version_value = (jstring)(env->CallStaticObjectMethod(System,System_getProperty,vm_version_name));

        char * cvm_version_value = (char *)env->GetStringUTFChars(vm_version_value,NULL);
        double version = atof(cvm_version_value);
        g_isArt = version >= 2 ? true: false;
        LOGD("[+]Android VmVersion:%f",version);

        env->ReleaseStringUTFChars(vm_version_value,cvm_version_value);
        env->DeleteLocalRef(System);
        env->DeleteLocalRef(vm_version_name);
        env->DeleteLocalRef(vm_version_value);
    }else{
        LOGD("[-]unsupported Android version");
        assert(false);
    }
    jniRegisterNativeMethods(env,"com/android/shell/Native",methods,sizeof(methods)/sizeof(methods[0]));
    env->DeleteLocalRef(jclazz);
}

__attribute__((visibility("default"))) JNIEXPORT jint JNI_OnLoad(JavaVM * vm,void * reversed)
{
    JNIEnv * env = NULL;

    if(vm->GetEnv((void **)&env,JNI_VERSION_1_6) != JNI_OK)
    {
        return -1;
    }
    init(env);//调用init
    return  JNI_VERSION_1_6;
}