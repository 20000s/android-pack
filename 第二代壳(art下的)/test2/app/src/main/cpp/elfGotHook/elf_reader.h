#ifndef INJECTDEMO_ELF_READER_H
#define INJECTDEMO_ELF_READER_H

#include <string.h>
#include <stdio.h>

#include "def.h"
#include "logger.h"

class ElfReader
{
public:
    ElfReader(const char * module,void * start);
    int parse();
    int hook(const char * func_name,void * new_func,void ** old_func);
    void dumpElfHeader();//elf的头
    void dumpProgramHeaders(); //elf的程序头
    void dumpDynamicSegment();//动态链接段
    void dumpDynamicRel(); // 动态链接的重定位

private:
    int verifyElfHeader();
    int elfTargetMachine();
    ElfW(Addr) getSegmentBaseAddress();
    ElfW(Phdr) * findSegmentByType(ElfW(Word) type);
    ElfW(Phdr) * findSegmentByAddress(void * addr);
    int parseDynamicSegment();//解析段的
    int findSymbolByName(const char * symbol , ElfW(Sym) ** sym,uint32_t * symidx);
    int elfLookup(const char * symbol , ElfW(Sym) ** sym,uint32_t * symidx);//查找
    int gnuLookup(const char * symbol , ElfW(Sym) ** sym,uint32_t * symidx);
    int hookInternally(void * addr,void * new_func,void ** old_func); //hook的

private:
    const char *moduleName; // 要hook的模块名
    ElfW(Addr) start;
    ElfW(Addr) bias;//真实的加载地址（mmap分配）与计算出来的（读ELF程序头中的p_vaddr，根据页对齐）加载地址之差 在加上basesaddress(真实加载的baseaddress)
    ElfW(Ehdr) * ehdr;
    ElfW(Half) phdrNum; // 程序头的数量
    ElfW(Phdr) * phdr;

    const char * strTable = NULL;
    ElfW(Sym) * symTable = NULL;
    ElfW(Xword) relCount = 0;
    ElfW(Xword) pltRelCount = 0;

#if defined(USE_RELA)
    ElfW(Rela) *rel = NULL;
    ElfW(Rela) *pltRel = NULL;
#else
    ElfW(Rel) *rel = NULL;
    ElfW(Rel) *pltRel = NULL;
#endif

    //for elf hash
    uint32_t nbucket;
    uint32_t nchain;
    uint32_t *bucket;
    uint32_t *chain;

    //for gnu hash
    bool isGnuHash = false;
    uint32_t gnuNBucket;
    uint32_t gnuSymndex;
    uint32_t gnuMaskwords;
    uint32_t gnuShift2;
    uint32_t *gnuBucket;
    uint32_t *gnuChain;
    ElfW(Addr) * gnuBloomFilter;
};


#endif