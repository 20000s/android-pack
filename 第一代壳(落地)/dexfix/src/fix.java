import java.io.*;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.zip.Adler32;

public class fix {
    public static void main(String[] args) {

        if(args[0].equals("-h"))
        {
           System.out.println("把欲加壳的apk追加到dex文件中");
           System.out.println("参数： [想要加壳的apk][壳apk的dex文件]");
           return;
        }
        else if(args.length !=2)
        {
            System.out.println("\r\n请输入正确的参数");
            System.out.println("参数： [想要加壳的apk][壳apk的dex文件]");
            return;
        }
        try{
            File apkfile = new File(args[0]);
            System.out.println("原理apk文件的大小为" + apkfile.length() + " byte");

            File shellDexFile = new File(args[1]);
            System.out.println("处理壳..");
            byte[] shellDexFileBuffer= readFileBytes(shellDexFile);
            System.out.println("壳的apk大小为：" + shellDexFileBuffer.length + "bytes");

            //将原始的apk进行加密
            System.out.println("正在加密apk文件");
            byte[] encryptedAPKFileBuffer = encrypt(readFileBytes(apkfile));
            System.out.println("apk文件加密完成");

            //构建新的dex
            System.out.println("开始构建新的dex");
            int lengthOfEncryptedAPK = encryptedAPKFileBuffer.length;
            int shellDexFileLength = shellDexFileBuffer.length;
            int newDexFileLength = lengthOfEncryptedAPK + shellDexFileLength + 4;
            System.out.println("新的dex文件大小为：" + newDexFileLength + "bytes");
            byte[] newDexFileBuffer = new byte[newDexFileLength];

            //复制壳的dex
            System.out.println("复制dex文件");
            System.arraycopy(shellDexFileBuffer,0,newDexFileBuffer,0,shellDexFileLength);
            System.arraycopy(encryptedAPKFileBuffer,0,newDexFileBuffer,shellDexFileLength,lengthOfEncryptedAPK);
            System.arraycopy(intToByte(lengthOfEncryptedAPK),0,newDexFileBuffer,newDexFileLength-4,4);

            System.out.println("正在校正新的dex文件");
            fixFileSizeHeader(newDexFileBuffer);
            fixSHA1Header(newDexFileBuffer);
            fixCheckSumHeader(newDexFileBuffer);
            System.out.println("校正完了...");

            System.out.println("保存文件。。");
            String newDexFileName = args[1];
            File file = new File(newDexFileName);
            if(!file.exists()){
                file.createNewFile();
            }

            FileOutputStream localFileOutputStream = new FileOutputStream(newDexFileName);
            localFileOutputStream.write(newDexFileBuffer);
            localFileOutputStream.flush();
            System.out.println("保存完成");
            localFileOutputStream.close();
        }catch (Exception e){
            e.printStackTrace();
        }
    }
    // 自行选用加密方式
    private static byte[] encrypt(byte[] srcdata){
        for(int i = 0;i<srcdata.length;i++){
            srcdata[i] = (byte)(0xFF ^ srcdata[i]);
        }
        return srcdata;
    }

    // 修改dex头 file_size值
    private static void fixFileSizeHeader(byte[] dexBytes) {
        byte[] newFileBuffer = intToByte(dexBytes.length);
        byte[] refs = new byte[4];
        for (int i = 0; i < 4; i++) {
            refs[i] = newFileBuffer[newFileBuffer.length - 1 - i];
        }
        System.arraycopy(refs, 0, dexBytes, 32, 4);

        String hexstr = "";
        for (int i = 0; i < refs.length; i++) {
            hexstr += Integer.toString((refs[i] & 0xff) + 0x100, 16)
                    .substring(1);
        }
        System.out.println("FileSize：" + hexstr);
    }

    // 修改dex头 sha1值
    private static void fixSHA1Header(byte[] dexBytes) throws NoSuchAlgorithmException {
        MessageDigest messageDigest = MessageDigest.getInstance("SHA-1");
        messageDigest.update(dexBytes, 32, dexBytes.length - 32);
        byte[] newSha1Value = messageDigest.digest();
        System.arraycopy(newSha1Value, 0, dexBytes, 12, 20);//修改sha-1值（12-31）

        // 输出sha-1值
        String hexstr = "";
        for (int i = 0; i < newSha1Value.length; i++) {
            hexstr += Integer.toString((newSha1Value[i] & 0xff) + 0x100, 16)
                    .substring(1);
        }
        System.out.println("SHA1：" + hexstr);
    }

    // 修改dex头，CheckSum 校验码
    private static void fixCheckSumHeader(byte[] dexBytes) {
        Adler32 adler = new Adler32();

        // 从12到文件末尾计算校验码
        adler.update(dexBytes, 12, dexBytes.length - 12);
        long value = adler.getValue();
        int va = (int) value;
        byte[] newcs = intToByte(va);
        byte[] recs = new byte[4];
        for (int i = 0; i < 4; i++) {
            recs[i] = newcs[newcs.length - 1 - i];
        }
        System.arraycopy(recs, 0, dexBytes, 8, 4); //效验码赋值（8-11）

        System.out.println("CheckSum: " + Long.toHexString(value));
    }

    // int ==> byte[]
    public static byte[] intToByte(int number) {
        byte[] b = new byte[4];
        for (int i = 3; i >= 0; i--) {
            b[i] = (byte) (number % 256);
            number >>= 8;
        }
        return b;
    }

    // 以二进制读出文件内容
    private static byte[] readFileBytes(File file) throws IOException {
        byte[] arrayOfByte = new byte[1024];
        ByteArrayOutputStream localByteArrayOutputStream = new ByteArrayOutputStream();
        FileInputStream fis = new FileInputStream(file);
        while (true) {
            int i = fis.read(arrayOfByte);
            if (i != -1) {
                localByteArrayOutputStream.write(arrayOfByte, 0, i);
            } else {
                return localByteArrayOutputStream.toByteArray();
            }
        }
    }

}
