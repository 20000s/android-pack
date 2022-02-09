# 安卓安全加固

## 第一代壳

**原理实现**：

![img](https://bbs.pediy.com/upload/attach/202104/922119_S5AS85RG82HU74U.jpg)

**背景知识**：

### ***1.classloader：***



类加载器，听名字就知道用于加载类的，对于安卓层面的话，就是用于加载dex文件

### ***2.双亲委派*：**

加载dex文件的时候，问一下当前的类加载器有没有加载，没的话，递归问类加载器的超类,这一继承路上，只要dex文件被其中一个classloader加载，其他时候都不用被加载 android用的就是java的，所以一样，不过java是class,android是dex

### *3.android的类加载器*：

```
1.bootclassloader（加载android框架的）
2.pathclassloader（加载安装到android上的apk文件）
3.dexclassloader(可以加载自己的dex文件)，动态加载的关键点
4.baseclassloader(pathclassloader和dexclassloader的父类)
```

### *4.pathclassloader和dexclassloader区别*

其实没什么区别，毕竟父类都是baseclassloader,唯一区别在于dexclassloader比pathclassloader多一个存放自己dex文件的参数，用于加载classloader的时候

```
// PathClassLoader
public class PathClassLoader extends BaseDexClassLoader {
    public PathClassLoader(String dexPath, ClassLoader parent) {
        super(dexPath, null, null, parent);
    }

    public PathClassLoader(String dexPath, String librarySearchPath, ClassLoader parent) {
        super(dexPath, null, librarySearchPath, parent);
    }
}
// DexClassLoader
public class DexClassLoader extends BaseDexClassLoader {
    public DexClassLoader(String dexPath, String optimizedDirectory,
            String librarySearchPath, ClassLoader parent) {
        super(dexPath, new File(optimizedDirectory), librarySearchPath, parent);
    }
}
```

PathClassLoader与DexClassLoader都继承于BaseDexClassLoader。

PathClassLoader与DexClassLoader在构造函数中都调用了父类的构造函数，但DexClassLoader多传了一个optimizedDirectory。(android 8后删除)

### *5.BaseDexClassLoader*:

```
public class BaseDexClassLoader extends ClassLoader {
    private final DexPathList pathList;
    ...
    public BaseDexClassLoader(String dexPath, File optimizedDirectory, String libraryPath, ClassLoader parent){
        super(parent);
        this.pathList = new DexPathList(this, dexPath, libraryPath, optimizedDirectory);
    }
    ...
}
```

### *6.android如何判断当前的dex已经加载？（findclass方法）*

PathClassLoader和DexClassLoader源码中都没有重写父类的findClass()方法，但它们的父类BaseDexClassLoader就有重写findClass()

```
private final DexPathList pathList;

@Override
protected Class<?> findClass(String name) throws ClassNotFoundException {
    List<Throwable> suppressedExceptions = new ArrayList<Throwable>();
    // 实质是通过pathList的对象findClass()方法来获取class
    Class c = pathList.findClass(name, suppressedExceptions);
    if (c == null) {
        ClassNotFoundException cnfe = new ClassNotFoundException("Didn't find class \"" + name + "\" on path: " + pathList);
        for (Throwable t : suppressedExceptions) {
            cnfe.addSuppressed(t);
        }
        throw cnfe;
    }
    return c;
}
```

BaseDexClassLoader的findClass()方法实际上是通过DexPathList对象（pathList）的findClass()方法来获取class的，而这个DexPathList对象恰好在之前的BaseDexClassLoader构造函数中就已经被创建好了(前面写的basedexclassloader里的)

### *7.DexPathList*:

```
private final Element[] dexElements;

public DexPathList(ClassLoader definingContext, String dexPath,
        String libraryPath, File optimizedDirectory) {
    ...
    this.definingContext = definingContext;
    this.dexElements = makeDexElements(splitDexPath(dexPath), optimizedDirectory,suppressedExceptions);
    ...
}


```

构造函数中，保存了当前的类加载器definingContext，并调用了makeDexElements()得到Element集合。

```
private static Element[] makeDexElements(ArrayList<File> files, File optimizedDirectory, ArrayList<IOException> suppressedExceptions) {
    // 1.创建Element集合
    ArrayList<Element> elements = new ArrayList<Element>();
    // 2.遍历所有dex文件（也可能是jar、apk或zip文件）
    for (File file : files) {
        ZipFile zip = null;
        DexFile dex = null;
        String name = file.getName();
        ...
        // 如果是dex文件
        if (name.endsWith(DEX_SUFFIX)) {
            dex = loadDexFile(file, optimizedDirectory);

        // 如果是apk、jar、zip文件（这部分在不同的Android版本中，处理方式有细微差别）
        } else {
            zip = file;
            dex = loadDexFile(file, optimizedDirectory);
        }
        ...
        // 3.将dex文件或压缩文件包装成Element对象，并添加到Element集合中
        if ((zip != null) || (dex != null)) {
            elements.add(new Element(file, false, zip, dex));
        }
    }
    // 4.将Element集合转成Element数组返回
    return elements.toArray(new Element[elements.size()]);
}

```

DexPathList的构造函数是将一个个的程序文件（可能是dex、apk、jar、zip）封装成一个个Element对象，最后添加到Element集合中。

**dexpathlist的findclass()**

```
public Class findClass(String name, List<Throwable> suppressed) {
    for (Element element : dexElements) {
        // 遍历出一个dex文件
        DexFile dex = element.dexFile;

        if (dex != null) {
            // 在dex文件中查找类名与name相同的类
            Class clazz = dex.loadClassBinaryName(name, definingContext, suppressed);
            if (clazz != null) {
                return clazz;
            }
        }
    }
    if (dexElementsSuppressedExceptions != null) {
        suppressed.addAll(Arrays.asList(dexElementsSuppressedExceptions));
    }
    return null;
}

```

结合DexPathList的构造函数，其实DexPathList的findClass()方法很简单，就只是对Element数组进行遍历，一旦找到类名与name相同的类时，就直接返回这个class，找不到则返回null。

#### 总结

当我们使用dexclassloader加载dex文件时,dexclassloader调用它的超类baseclassloader,
而baseclassloader由此new dexpathlist,接着dexpathlist使用makedexelements方法加载dex文件，同时将加载好后的文件包装成elements中去，而类加载器使用findclass判断是否加载过该dex文件时，正是通过对elements集合的遍历查找

额外的知识：

加载Activity的时候，有一个很重要的类：**LoadedApk.java**，这个类是负责加载一个Apk程序的，

他内部有一个**mClassLoader**变量，他就是负责加载一个Apk程序的，那么我们只要获取到这个类加载器就可以了。他不是static的，所以我们还得获取一个LoadedApk对象

ActivityThread类中有一个自己的static对象，然后**还有一个ArrayMap存放Apk包名和LoadedApk映射关系的数据结构mpackages**，

### 实现

apk运行靠的是application对象，这是程序运行之后的全局类，所以必须找到源apk的application类，运行它的oncreate方法，

还有一个问题，需要一个时机在脱壳程序还没运行起来的时候来加载源程序的apk,这个时机不能太晚，不然的话，就是运行脱壳程序，而不是源程序，application的attachbasecontext方法在application的on create方法执行前执行，所以加载apk的重任就落在了attachbasecontext上了





之后在application的oncreate方法中替换application就好了。替换成源程序的application，对了 还有资源问题，由于我比较懒。。。所以资源加载 壳的资源和源程序的资源是一摸一样的，，，

具体代码实现看github



