//
// Created by 24657 on 2021/4/29.
//

#include "byte_load.h"
#include <jni.h>
#include <android/log.h>
#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <cstdlib>
#include <sys/stat.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <vector>
#include <memory.h>
#include "loaddex.h"
#include "dex_header.h"
#include "elfGotHook/logger.h"
#include "utils.h"

using namespace std;


void *mem_loadDex_byte19(void *arthandler, const char *base, size_t size)
{
    string location = "";
    string err_msg;
    org1_artDexFileOpenMemory19 func =  (org1_artDexFileOpenMemory19)dlsym(arthandler,
                                                                           "_ZN3art7DexFile10OpenMemoryEPKhjRKSsjPNS_6MemMapE");//openmemeory
     if(!func)
     {
         LOGE("[-]dlsym open Memory failed: %s",dlerror());
     }
     LOGD("[+] call openMemory function");
     const Header * dex_header = reinterpret_cast<const Header *>(base);
     void *value = func((const unsigned char *)base,size,location,dex_header->checksum_,NULL);//加载
     if(!value)
     {
         LOGE("[-] sdk_int: %d call openMemory failed: %s",g_sdk_int,dlerror());
         return NULL;
     }
    return value;

}


void *mem_loadDex_byte21(void *artHandle, const char *base,
                         size_t size)
{
    std::string location = "";
    std::string err_msg;
    org_artDexFileOpenMemory21 func = (org_artDexFileOpenMemory21)dlsym(artHandle,
                                                                        "_ZN3art7DexFile10OpenMemoryEPKhjRKNSt3__112basic_stringIcNS3_11char_traitsIcEENS3_9allocatorIcEEEEjPNS_6MemMapEPS9_");

    if (!func)
    {
        return NULL;
    }
    const Header *dex_header = reinterpret_cast<const Header *>(base);
    void *value = func((const unsigned char *)base,
                       size,
                       location,
                       dex_header->checksum_,
                       NULL,
                       &err_msg);

    if (!value)
    {
        LOGE("[-]sdk_int:%d dlsym openMemory failed:%s", g_sdk_int, dlerror());
        return NULL;
    }

    return value;
}

void *mem_loadDex_byte22(void *artHandle, const char *base, size_t size)
{
    std::string location = "";
    std::string err_msg;

    org_artDexFileOpenMemory22 func = (org_artDexFileOpenMemory22)dlsym(artHandle,
                                                                        "_ZN3art7DexFile10OpenMemoryEPKhjRKNSt3__112basic_stringIcNS3_11char_traitsIcEENS3_9allocatorIcEEEEjPNS_6MemMapEPKNS_7OatFileEPS9_");

    if (!func)
    {
        LOGE("[-]sdk_int:%d dlsym openMemory failed:%s", g_sdk_int, dlerror());
        return NULL;
    }
    const Header *dex_header = reinterpret_cast<const Header *>(base);
    // It is not cookie
    void *value = func((const unsigned char *)base,
                       size,
                       location,
                       dex_header->checksum_,
                       NULL,
                       NULL,
                       &err_msg);

    if (!value)
    {
        LOGE("[-]call artDexFileOpenMemory22 failed");
        return NULL;
    }
    LOGD("[+]openMemory value:%p", value);
    return value;
} // mem_loadDex_byte22



/**
 * [mem_loadDex_byte23 description]
 * @param artHandle [description]
 * @param base      [description]
 * @param size      [description]
 */
void *mem_loadDex_byte23(void *artHandle, const char *base, size_t size)
{

    std::string location = "";
    std::string err_msg;

    void *retcookie = malloc(0x78);
    memset(retcookie, 0, (size_t)0x78);
    org_artDexFileOpenMemory23 func = (org_artDexFileOpenMemory23)dlsym(artHandle,
                                                                        "_ZN3art7DexFile10OpenMemoryEPKhjRKNSt3__112basic_stringIcNS3_11char_traitsIcEENS3_9allocatorIcEEEEjPNS_6MemMapEPKNS_10OatDexFileEPS9_");

    if (!func)
    {
        LOGE("[-]sdk_int:%d dlsym openMemory failed:%s", g_sdk_int, dlerror());
        return NULL;
    }

    const Header *dex_header = reinterpret_cast<const Header *>(base);
    void *value = func(retcookie,
                       (const unsigned char *)base,
                       size,
                       location,
                       dex_header->checksum_,
                       NULL,
                       NULL,
                       &err_msg);

    void *a = retcookie;

    // 返回的value等于retcookie ,*(int*)retcookie存储了加载dex的cookie
    LOGD("[+]openmemory value:%p,*retcookie:%x,jlong* retcookie:%llx", value, *(int *)retcookie, *(jlong *)a);
    // (*(jlong*)retcookie和*(int*)retcookie相等
    return (void *)(*(jlong *)retcookie);
}

void *mem_loadDex_byte24(void *artHandle, const char *base, size_t size)
{
    std::string location = "";
    std::string err_msg;
    void *retcookie = malloc(0x78);
    memset(retcookie, 0, 0x78);

    // #define SEARCH_SYMBOL_Nougat																																			_ZN3art7DexFile10OpenMemoryEPKhjRKNSt3__112basic_stringIcNS3_11char_traitsIcEENS3_9allocatorIcEEEEjPNS_6MemMapEPKNS_10OatDexFileEPS9_
    org_artDexFileOpenMemory23 func = (org_artDexFileOpenMemory23)dlsym(artHandle,
                                                                        "_ZN3art7DexFile10OpenMemoryEPKhjRKNSt3__112basic_stringIcNS3_11char_traitsIcEENS3_9allocatorIcEEEEjPNS_6MemMapEPKNS_10OatDexFileEPS9_");

    if (!func)
    {
        LOGE("[-]sdk_int:%d dlsym openMemory failed:%s", g_sdk_int, dlerror());
#ifndef SEARCH_SYMBOL_Nougat
        return NULL;
#else  // ifndef SEARCH_SYMBOL_Nougat
        LOGD("[+]try search symbol from elf file");
        func = (org_artDexFileOpenMemory23)get_addr_symbol("/system/lib/libart.so",
                                                           "_ZN3art7DexFile10OpenMemoryEPKhjRKNSt3__112basic_stringIcNS3_11char_traitsIcEENS3_9allocatorIcEEEEjPNS_6MemMapEPKNS_10OatDexFileEPS9_");

        if (!func)
        {
            LOGE("[-]search symbol openMemory uniptr failed");
            return NULL;
        }
#endif // ifndef SEARCH_SYMBOL_Nougat
    }
    const Header *dex_header = reinterpret_cast<const Header *>(base);
    void *value = func(retcookie,
                       (const unsigned char *)base,
                       size,
                       location,
                       dex_header->checksum_,
                       NULL,
                       NULL,
                       &err_msg);
    void *a = retcookie;

    // LOGD改变了retcookie？？ 所以先用a备份
    LOGD("[+]openMemory value:%p,*(int*)retcookie:%x,*(jlong*)retcookie:%llx",
         value,
         *(int *)retcookie,
         *(jlong *)a);

    return (void *)(*(jlong *)retcookie);
} // mem_loadDex_byte2