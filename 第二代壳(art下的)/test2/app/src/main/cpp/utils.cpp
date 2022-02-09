//
// Created by 24657 on 2021/4/29.
//

#include "utils.h"
#include <jni.h>
#include <dlfcn.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "elfGotHook/logger.h"

int extract_file(JNIEnv *env,jobject ctx,const char * szDexPath,const  char * fileName)
{
    if(access(szDexPath,F_OK) == 0)//判断文件是否存在，如果存在，说明已经提取出来了，没有就提取
    {
        LOGD("[+]File %s have existed",szDexPath);
        return 0;
    }
    //不存在，开始提取
    else{
        AAssetManager *mgr;
        jclass ApplicationClass = env->GetObjectClass(ctx);
        jmethodID getAsets = env->GetMethodID(ApplicationClass,"getAssets","()Landroid/content/res/AssetManager;");
        jobject Assets_obj = env->CallObjectMethod(ctx,getAsets);
        mgr = AAssetManager_fromJava(env,Assets_obj); //得到assetManager
        if(mgr == NULL)
        {
            LOGE("[-]getAAsetManager failed");
            return 0;
        }
        AAsset  * asset = AAssetManager_open(mgr,fileName,AASSET_MODE_STREAMING);
        FILE * file = fopen(szDexPath,"wb");//打开文件
        int bufferSize = AAsset_getLength(asset);
        LOGD("[+]Asser FileName: %s,extract path: %s,size: %d\n",fileName,szDexPath,bufferSize);
        void * buffer = malloc(4096);
        while (true){
            int numBytesRead = AAsset_read(asset,buffer,4096);//写进uufer
            if(numBytesRead <= 0)
            {
                break;
            }
            fwrite(buffer,numBytesRead,1,file);// buffer写进file里
        }
        free(buffer);
        fclose(file);
        AAsset_close(asset);
        chmod(szDexPath,493);


    }
}  //提取文件
void * get_module_base(pid_t pid,const char * module_name)
{
    FILE *fp;
    long addr = 0;
    char * pch;
    char filename[32];
    char line[1024];

    if(pid < 0)
    {
        snprintf(filename,sizeof(filename),"/proc/self/maps",pid);  //自身进程
    }
    else{
        snprintf(filename,sizeof(filename),"/proc/%d/maps",pid); //别的进程
    }

    fp = fopen(filename,"r");

    if(fp != NULL)
    {
        while(fgets(line,sizeof(line),fp))
        {
            if(strstr(line,module_name))//so的基地址
            {
                pch = strtok(line,"-");
                addr = strtoull(pch,NULL,16);

                if(addr == 0x8000)
                {
                    addr = 0;
                }
                break;
            }
        }
        fclose(fp);
    }
    return  (void*)addr;
}
