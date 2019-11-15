package utfdecode.tests;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import java.text.Normalizer;

class NormalizationTest {

    @Test void basic() {
        var output = Utfdecode.getUtf8Output("å", Normalizer.Form.NFD);
        Assertions.assertEquals("a\u030a", output);
        output = Utfdecode.getUtf8Output("å", Normalizer.Form.NFKD);
        Assertions.assertEquals("a\u030a", output);

        // U+FB00, typographic ligature "ﬀ"
        output = Utfdecode.getUtf8Output("\uFB00", Normalizer.Form.NFD);
        Assertions.assertEquals("\uFB00", output);
        output = Utfdecode.getUtf8Output("\uFB00", Normalizer.Form.NFKD);
        Assertions.assertEquals("ff", output);

        // U+212B ANGSTROM SIGN
        output = Utfdecode.getUtf8Output("\u212B", Normalizer.Form.NFD);
        Assertions.assertEquals("A\u030a", output);
        output = Utfdecode.getUtf8Output("\u212B", Normalizer.Form.NFKD);
        Assertions.assertEquals("A\u030a", output);
    }

}
