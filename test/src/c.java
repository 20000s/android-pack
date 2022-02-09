import java.math.BigDecimal;

public class c {
    public String[] Q;
    public BigDecimal[] R;
    public BigDecimal[] S;

    public static BigDecimal a(BigDecimal[][] arg6) {
        return arg6[0][0].multiply(arg6[1][1]).multiply(arg6[2][2]).subtract(arg6[0][0].multiply(arg6[1][2]).multiply(arg6[2][1])).subtract(arg6[0][1].multiply(arg6[1][0]).multiply(arg6[2][2])).add(arg6[0][1].multiply(arg6[1][2]).multiply(arg6[2][0])).add(arg6[0][2].multiply(arg6[1][0]).multiply(arg6[2][1])).subtract(arg6[0][2].multiply(arg6[1][1]).multiply(arg6[2][0]));
    }
}
