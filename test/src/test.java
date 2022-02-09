import java.io.BufferedReader;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.math.BigDecimal;
import java.math.BigInteger;
import java.nio.charset.StandardCharsets;
import java.security.InvalidAlgorithmParameterException;
import java.security.InvalidKeyException;
import java.security.Key;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.NoSuchProviderException;
import java.security.SecureRandom;
import java.security.UnrecoverableEntryException;
import java.security.cert.CertificateException;
import java.security.spec.InvalidKeySpecException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import javax.crypto.*;
import javax.crypto.spec.PBEParameterSpec;

public class test {
    public static void main(String[] args) throws InvalidKeySpecException, NoSuchAlgorithmException, NoSuchPaddingException, BadPaddingException, IllegalBlockSizeException, InvalidKeyException, NoSuchProviderException, InvalidAlgorithmParameterException {
        String v1 = "1283034116048065701_d0UubuJ/c2WtfOYuP1goJJFqkvJOV1gZTZZwd3Ft4ocQeHbMSdvrc4LjHgOdIa90";
        String[] v7 = new String[4];
        c v2 = new c();
        v7[0] = "31627_2463002213219449031157189171389412";
        v7[1] = "60334_2463002213719983401596195542989712";
        v7[2] = "50033_2463002213468695035757250868133120";
        v7[3] = "63748_2463002213824972784058087693515910";
         v2.Q = v7;

        BigDecimal[] v4_3 = new BigDecimal[7];
        BigDecimal[] v3_2 = new BigDecimal[7];
        int v7_1;
        for(v7_1 = 0; true; ++v7_1) {
            String[] v8_4 = v2.Q;
            if(v7_1 >= v8_4.length) {
                break;
            }

            String[] v8_5 = v8_4[v7_1].split("_");
            v4_3[v7_1] = new BigDecimal(v8_5[0]);
            v3_2[v7_1] = new BigDecimal(v8_5[1]);
        }

        v2.R = v4_3;
        v2.S = v3_2;


        BigDecimal[][] v3_3 = new BigDecimal[4][4];
        BigDecimal[][] v4_4 = new BigDecimal[4][4];
        BigDecimal v7_2 = new BigDecimal("1");
        int v9_1;
        for(v9_1 = 0; v9_1 < 4; ++v9_1) {
            int v11;
            for(v11 = 0; v11 < 4; ++v11) {
                if(v11 == 0) {
                    v3_3[v9_1][v11] = v2.S[v9_1];
                    v4_4[v9_1][v11] = v7_2;
                }
                else {
                    v3_3[v9_1][v11] = v2.R[v9_1].pow(v11);
                    v4_4[v9_1][v11] = v2.R[v9_1].pow(v11);
                }
            }
        }

        BigDecimal[] v2_1 = new BigDecimal[4];
        BigDecimal v7_3 = new BigDecimal("1");
        int v8_6;
        for(v8_6 = 0; v8_6 < 4; ++v8_6) {
            v2_1[v8_6] = v4_4[v8_6][1];
        }

        int v4_5 = 1;
        Arrays.sort(((Object[])v2_1));
        int v6 = 3;
        while(v6 >= v4_5) {
            int v4_6 = v6 - 1;
            int v8_7;
            for(v8_7 = v4_6; v8_7 >= 0; --v8_7) {
                v7_3 = v7_3.multiply(v2_1[v6].subtract(v2_1[v8_7]));
            }

            v6 = v4_6;
            v4_5 = 1;
        }

        BigDecimal[][] v2_2 = new BigDecimal[3][3];
        BigDecimal[][] v4_7 = new BigDecimal[3][3];
        BigDecimal[][] v6_1 = new BigDecimal[3][3];
        BigDecimal[][] v8_8 = new BigDecimal[3][3];
        int v9_2;
        for(v9_2 = 0; v9_2 < v3_3.length; ++v9_2) {
            int v11_1;
            for(v11_1 = 0; v11_1 < v3_3.length; ++v11_1) {
                if(v9_2 != 0 && v11_1 != 0) {
                    v2_2[v9_2 - 1][v11_1 - 1] = v3_3[v9_2][v11_1];
                }

                if(v9_2 != 0 && v11_1 != 1) {
                    if(v11_1 == 0) {
                        v4_7[v9_2 - 1][v11_1] = v3_3[v9_2][v11_1];
                    }
                    else {
                        v4_7[v9_2 - 1][v11_1 - 1] = v3_3[v9_2][v11_1];
                    }
                }

                if(v9_2 != 0 && v11_1 != 2) {
                    if((v11_1 == 1 ? 1 : 0 | (v11_1 == 0 ? 1 : 0)) == 0) {
                        v6_1[v9_2 - 1][v11_1 - 1] = v3_3[v9_2][v11_1];
                    }
                    else {
                        v6_1[v9_2 - 1][v11_1] = v3_3[v9_2][v11_1];
                    }
                }

                if(v9_2 != 0 && v11_1 != 3) {
                    v8_8[v9_2 - 1][v11_1] = v3_3[v9_2][v11_1];
                }
            }
        }

        BigDecimal v2_3 = v3_3[0][0].multiply(c.a(v2_2)).subtract(v3_3[0][1].multiply(c.a(v4_7))).add(v3_3[0][2].multiply(c.a(v6_1))).subtract(v3_3[0][3].multiply(c.a(v8_8)));
        if(v2_3.doubleValue() < 0) {
            v7_3 = v7_3.multiply(new BigDecimal("-1"));
        }
        System.out.println(v2_3.divide(v7_3, 4));
        String j0 = new String(v2_3.divide(v7_3, 4).toBigInteger().toByteArray());
        String v2_5 = j0;
        String v3_5 = v1.split("_")[1];
        a.fg = new BigInteger(v1.split("_")[0]).toByteArray();
        System.out.println("fdfdgf"+v3_5);
        byte[] v1_1 = Base64.decode(v3_5.getBytes(StandardCharsets.US_ASCII), 2);
        byte[] salt = new byte[8];
        PBEParameterSpec paramSpec = new PBEParameterSpec(salt, 0);
        Key v2_6 = a.a(v2_5);
        Cipher v3_6 = Cipher.getInstance("PBEWITHSHA256AND128BITAES-CBC-BC","BC");
       System.out.println(v3_5.getBytes(StandardCharsets.US_ASCII).length);

        v3_6.init(2, v2_6,paramSpec);
        System.out.println(v1_1.length);
        System.out.println("Flag decryption successfully！\n" + new String(v3_6.doFinal(v1_1)) + "\n");
    }
}
