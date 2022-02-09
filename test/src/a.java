

import javax.crypto.SecretKey;
import javax.crypto.SecretKeyFactory;
import javax.crypto.spec.PBEKeySpec;
import java.security.NoSuchAlgorithmException;
import java.security.NoSuchProviderException;
import java.security.Security;
import java.security.spec.InvalidKeySpecException;
import org.bouncycastle.jce.provider.BouncyCastleProvider;
public class a {
    public static byte[] fg = null;
    public static int b = 10;


    public static SecretKey a(String arg4) throws NoSuchAlgorithmException, InvalidKeySpecException, NoSuchProviderException {
        PBEKeySpec v0 = new PBEKeySpec(arg4.toCharArray(),a.fg , 10000, 0x80);
        String cryptoAlgorithm = "PBEWITHSHA256AND128BITAES-CBC-BC";
        Security.addProvider(new BouncyCastleProvider());

        char[] passPhrase = null;
        passPhrase = "q1dd".toCharArray();
        PBEKeySpec pbeKeySpec = new PBEKeySpec(passPhrase);
        try {
            SecretKeyFactory secretKeyFactory = SecretKeyFactory.getInstance(cryptoAlgorithm, "BC");
            System.out.println(pbeKeySpec);
            SecretKey newSecretKey = secretKeyFactory.generateSecret(pbeKeySpec);
            assert newSecretKey != null;
            System.out.println("OK");
            return newSecretKey;
        } catch (NoSuchAlgorithmException e) {
            System.out.println("The algorithm is not found: " + cryptoAlgorithm);
        } catch (InvalidKeySpecException e) {
            System.out.println("The key spec is invalid");
        }
        System.out.println("retunr null");
         return  null;

    }
}

