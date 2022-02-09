hook关键函数

dalvik时代的dvmDexFileOpenPartial函数

![![img](https://bbs.pediy.com/upload/attach/202011/715334_3GX3W5QKVJTD3C9.png)](https://bbs.pediy.com/upload/attach/202011/715334_NMJMZZPPXNRM4UH.png)

6.0和7.0的openMemory函数

![img](https://bbs.pediy.com/upload/attach/202011/715334_3GX3W5QKVJTD3C9.png)

8.0~10.0的openCommon函数

![img](https://bbs.pediy.com/upload/attach/202011/715334_8G7H8SFRDY2W8RS.png)

这些函数其实也是只能针对整体壳的方式去进行dex的dump,对于指令提取壳不怎么行

，当然也可以全部调用一遍来补充。。不过就就是fart的原理了，frida脚本也可以实现

