//
// Created by 24657 on 2021/4/29.
//

#include "tools.h"
#include <string.h>


void * ElfHooker::get_module_base(pid_t pid,const char * module)
{
    char buffer[1024];

    if(pid <=  0)
    {
        snprintf(buffer,1024,"/proc/self/maps");
    }else{
        snprintf(buffer,1024,"/proc/%d/maps",pid);
    }
    //so的起始地点 终止地点
    unsigned long module_start;
    unsigned long module_end;
    char so_name[SO_NAME_LEN];
    FILE *mapsFile = fopen(buffer,"r");
    if(NULL != mapsFile)
    {
        while (fgets(buffer,1024,mapsFile))
        {

            if(sscanf(buffer,"%lx-%lx %*4s %*x %*x:%*x %*d %s",&module_start,&module_end,so_name) !=3)
            {
                continue;
            }

            if(0 == strncmp(so_name,module,strlen(module)))
            {
                fclose(mapsFile);
                return reinterpret_cast<void *>(module_start);
            }

        }


    }
    fclose(mapsFile);
    LOGD("failed to find module %s (%d)",module,pid);
    return  NULL;
}



uint32_t ElfHooker::elf_hash(const char * symbol)
{
    const uint8_t *name = reinterpret_cast<const uint8_t*>(symbol);//强制转化
    uint32_t h = 0,g;

    while(*name)
    {
        h = (h<<4) + *name++;
        g = h & 0xf0000000;
        h ^= g;
        h ^=g >>24;
    }
    return h;
}

uint32_t ElfHooker::gnu_hash(const char * symbol)
{
    uint32_t h = 5381;
    const uint8_t *name = reinterpret_cast<const uint8_t*>(symbol);

    while (0 != *name)
    {
        h += (h << 5) + *name++;
    }
    return h;
}


void ElfHooker::clear_cache(void * addr, size_t len)
{
    uint8_t *end = reinterpret_cast<uint8_t*>(addr) + len;

    syscall(0xf0002,addr,end);
}