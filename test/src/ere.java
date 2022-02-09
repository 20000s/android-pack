import org.bouncycastle.jce.provider.BouncyCastleProvider;

import javax.crypto.SecretKey;
import javax.crypto.SecretKeyFactory;
import javax.crypto.spec.PBEKeySpec;
import java.security.NoSuchAlgorithmException;
import java.security.Security;
import java.security.spec.InvalidKeySpecException;

public class ere {
    public static void main(String[] args) throws Exception{
        String cryptoAlgorithm = "PBEWITHSHA256AND128BITAES-CBC-BC";
        Security.addProvider(new BouncyCastleProvider());

        char[] passPhrase = null;
        passPhrase = "12321".toCharArray();
        PBEKeySpec pbeKeySpec = new PBEKeySpec(passPhrase);
        try {
            SecretKeyFactory secretKeyFactory = SecretKeyFactory.getInstance(cryptoAlgorithm, "BC");
            System.out.println(pbeKeySpec);
            SecretKey newSecretKey = secretKeyFactory.generateSecret(pbeKeySpec);
            assert newSecretKey != null;
            System.out.println("OK");
        } catch (NoSuchAlgorithmException e) {
            System.out.println("The algorithm is not found: " + cryptoAlgorithm);
        } catch (InvalidKeySpecException e) {
            System.out.println("The key spec is invalid");
        }
    }
}