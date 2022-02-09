# art模式下的dex文件加载流程

dal模式下的dex文件加载过程 写4.0的壳 感觉差不多了。。

先说一下结论，表层的java代码和dal模式下基本一毛一样，没什么区别，但是深入到原生层，完全不同。。。。。看c++代码看的头疼。。

从我们调用dexclasloader开始

![image-20210423143127434](C:\Users\24657\AppData\Roaming\Typora\typora-user-images\image-20210423143127434.png)

接着是basedexclassloader

![image-20210423143604345](C:\Users\24657\AppData\Roaming\Typora\typora-user-images\image-20210423143604345.png)

然后是dexpathlist,其中的makedexelememts是关键。。

![image-20210423143745296](C:\Users\24657\AppData\Roaming\Typora\typora-user-images\image-20210423143745296.png)

在makedexelements中 loaddexfile是重中之重

![image-20210423143908277](C:\Users\24657\AppData\Roaming\Typora\typora-user-images\image-20210423143908277.png)

loaddexfile调用dexfile.loaddex,新建new dexfile

![image-20210423144031584](C:\Users\24657\AppData\Roaming\Typora\typora-user-images\image-20210423144031584.png)

dexfile接着调用opendexfile

![image-20210423144439970](C:\Users\24657\AppData\Roaming\Typora\typora-user-images\image-20210423144439970.png)

opendexfile,接着调用nativedexfile

![image-20210423154101001](C:\Users\24657\AppData\Roaming\Typora\typora-user-images\image-20210423154101001.png)

接着调用opebdexfilesFromOat,这个是重点，接下来慢慢解析

```c++
bool ClassLinker::OpenDexFilesFromOat(const char* dex_location, const char* oat_location,
812                                      std::vector<std::string>* error_msgs,
813                                      std::vector<const DexFile*>* dex_files) {
814  // 1) Check whether we have an open oat file.
815  // This requires a dex checksum, use the "primary" one.
816  uint32_t dex_location_checksum;
817  uint32_t* dex_location_checksum_pointer = &dex_location_checksum;
818  bool have_checksum = true;
819  std::string checksum_error_msg;
820  if (!DexFile::GetChecksum(dex_location, dex_location_checksum_pointer, &checksum_error_msg)) {
821    // This happens for pre-opted files since the corresponding dex files are no longer on disk.
822    dex_location_checksum_pointer = nullptr;
823    have_checksum = false;
824  }
825
826  bool needs_registering = false;
827
828  const OatFile::OatDexFile* oat_dex_file = FindOpenedOatDexFile(oat_location, dex_location,
829                                                                 dex_location_checksum_pointer);
    //判断当前加载的dex文件有没有被加载过 （具体实现就不说了，第一次一定返回null）
830  std::unique_ptr<const OatFile> open_oat_file(
831      oat_dex_file != nullptr ? oat_dex_file->GetOatFile() : nullptr);
832
833  // 2) If we do not have an open one, maybe there's one on disk already.
834
835  // In case the oat file is not open, we play a locking game here so
836  // that if two different processes race to load and register or generate
837  // (or worse, one tries to open a partial generated file) we will be okay.
838  // This is actually common with apps that use DexClassLoader to work
839  // around the dex method reference limit and that have a background
840  // service running in a separate process.
    //如果没有加载了 判断可能还在磁盘 就基于对磁盘做一些操作
841  ScopedFlock scoped_flock;
842
843  if (open_oat_file.get() == nullptr) {//对磁盘的一些操作
844    if (oat_location != nullptr) {
845      // Can only do this if we have a checksum, else error.
846      if (!have_checksum) {
847        error_msgs->push_back(checksum_error_msg);
848        return false;
849      }
850
851      std::string error_msg;
852
853      // We are loading or creating one in the future. Time to set up the file lock.
854      if (!scoped_flock.Init(oat_location, &error_msg)) {
855        error_msgs->push_back(error_msg);
856        return false;
857      }
858
859      // TODO Caller specifically asks for this oat_location. We should honor it. Probably?
860      open_oat_file.reset(FindOatFileInOatLocationForDexFile(dex_location, dex_location_checksum,
861                                                             oat_location, &error_msg));
862
863      if (open_oat_file.get() == nullptr) {
864        std::string compound_msg = StringPrintf("Failed to find dex file '%s' in oat location '%s': %s",
865                                                dex_location, oat_location, error_msg.c_str());
866        VLOG(class_linker) << compound_msg;
867        error_msgs->push_back(compound_msg);
868      }
869    } else {
870      // TODO: What to lock here?
871      bool obsolete_file_cleanup_failed;
872      open_oat_file.reset(FindOatFileContainingDexFileFromDexLocation(dex_location,
873                                                                      dex_location_checksum_pointer,
874                                                                      kRuntimeISA, error_msgs,
875                                                                      &obsolete_file_cleanup_failed));
876      // There's no point in going forward and eventually try to regenerate the
877      // file if we couldn't remove the obsolete one. Mostly likely we will fail
878      // with the same error when trying to write the new file.
879      // TODO: should we maybe do this only when we get permission issues? (i.e. EACCESS).
880      if (obsolete_file_cleanup_failed) {
881        return false;
882      }
883    }
884    needs_registering = true;
885  }
    
    //运行到这里，无论之前有没有加载过，是一定加载好了的
    // 3) If we have an oat file, check all contained multidex files for our dex_location.
888  // Note: LoadMultiDexFilesFromOatFile will check for nullptr in the first argument.
889  bool success = LoadMultiDexFilesFromOatFile(open_oat_file.get(), dex_location,
890                                              dex_location_checksum_pointer,
891                                              false, error_msgs, dex_files);
    //LoadMultiDexFilesFromOatFile()是将多个dex文件（例如classes1.dex，classes2.dex）映射成DexFile对象，并添加到一个vector数据结构dex_files中，之后调用RegisterOatFile()函数将内存中的oat对象注册到oat_files_中，然后整个流程就跑完了。
892  if (success) {
893    const OatFile* oat_file = open_oat_file.release();  // Avoid deleting it.
894    if (needs_registering) {
895      // We opened the oat file, so we must register it.
896      RegisterOatFile(oat_file);
897    }
    //如果已经存在，LoadMultiDexFilesFromOatFile(...)后如果成功，则通过RegisterOatFile(...)加载到内存中，即oat_file;
898    // If the file isn't executable we failed patchoat but did manage to get the dex files.
899    return oat_file->IsExecutable();
900  } else {
   
901    if (needs_registering) {
902      // We opened it, delete it.
903      open_oat_file.reset();
904    } else {
905      open_oat_file.release();  // Do not delete open oat files.
906    }
907  }
908
909  // 4) If it's not the case (either no oat file or mismatches), regenerate and load.
910    //如果LoadMultDexFilesFromOatFile失败，重新生成 再加载一次
911  // Need a checksum, fail else.
912  if (!have_checksum) {
913    error_msgs->push_back(checksum_error_msg);
914    return false;
915  }
916
917  // Look in cache location if no oat_location is given.
918  std::string cache_location;
919  if (oat_location == nullptr) {
920    // Use the dalvik cache.
       //没看懂。。。。
921    const std::string dalvik_cache(GetDalvikCacheOrDie(GetInstructionSetString(kRuntimeISA)));
922    cache_location = GetDalvikCacheFilenameOrDie(dex_location, dalvik_cache.c_str());
923    oat_location = cache_location.c_str();
924  }
925//锁住干什么。。
926  bool has_flock = true;
927  // Definitely need to lock now.
928  if (!scoped_flock.HasFile()) {
929    std::string error_msg;
930    if (!scoped_flock.Init(oat_location, &error_msg)) {
931      error_msgs->push_back(error_msg);
932      has_flock = false;
933    }
934  }
935
936  if (Runtime::Current()->IsDex2OatEnabled() && has_flock && scoped_flock.HasFile()) {
937    // Create the oat file.
938    open_oat_file.reset(CreateOatFileForDexLocation(dex_location, scoped_flock.GetFile()->Fd(),
939                                                    oat_location, error_msgs));
940  }
941//生成oat文件 CreateOatFileForDexLocation
942  // Failed, bail.
943  if (open_oat_file.get() == nullptr) {
944    std::string error_msg;
945    // dex2oat was disabled or crashed. Add the dex file in the list of dex_files to make progress.
946    DexFile::Open(dex_location, dex_location, &error_msg, dex_files);
947    error_msgs->push_back(error_msg);
948    return false;
949  }
950//再次重新加载
951  // Try to load again, but stronger checks.
952  success = LoadMultiDexFilesFromOatFile(open_oat_file.get(), dex_location,
953                                         dex_location_checksum_pointer,
954                                         true, error_msgs, dex_files);
955  if (success) {
956    RegisterOatFile(open_oat_file.release());
957    return true;
958  } else {
959    return false;
960  }
961}
```



```c++
const OatFile* ClassLinker::CreateOatFileForDexLocation(const char* dex_location,
                                                        int fd, const char* oat_location,
                                                        std::vector<std::string>* error_msgs) {
  // Generate the output oat file for the dex file
  VLOG(class_linker) << "Generating oat file " << oat_location << " for " << dex_location;
  std::string error_msg;
  if (!GenerateOatFile(dex_location, fd, oat_location, &error_msg)) {
  //这里的GenerateOatFile(...)是重点，go
    CHECK(!error_msg.empty());
    error_msgs->push_back(error_msg);
    return nullptr;
  }
  std::unique_ptr<OatFile> oat_file(OatFile::Open(oat_location, oat_location, nullptr, nullptr,
                                            !Runtime::Current()->IsCompiler(),
                                            &error_msg));
  if (oat_file.get() == nullptr) {
    std::string compound_msg = StringPrintf("\nFailed to open generated oat file '%s': %s",
                                            oat_location, error_msg.c_str());
    error_msgs->push_back(compound_msg);
    return nullptr;
  }

  return oat_file.release();
}

```



总结：

表层的java的代码函数和dal模式的基本不存在任何区别，关键是最关键的原生代码，

首先判断在内存中是否有已经加载好了的oat(FindOpenedOatDexFile),如果没有在，再从磁盘里寻找dex相对应的oat文件oat(if里面的)，之后判断oat文件是否dex匹配，放到dexfiles,再注册（**LoadMultiDexFilesFromOatFile**） ,如果失败，重新生成dex，(CreateOatFileForDexLocation)使用dex2oat工具生成oat文件,再加载，注册

