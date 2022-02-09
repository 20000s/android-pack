在art模式下，没有现成函数供我们调用，这意味着要实现内存加载dex,相当的麻烦，得全部重写，

方法一：是hook openmemory但是mcookie不太好处理，而且7.0以上不可以

方法二: hook掉系统函数 fopen mmap等 在loaddex前hook ，将参数改为真实dex的地址,由于是系统函数

所以各版本通用，缺点是比较慢，而且极其难理解，（强烈建议在完全熟悉elf文件格式下仿写），本篇用

的是方法二

参考链接：

https://bbs.pediy.com/thread-225303.htm

