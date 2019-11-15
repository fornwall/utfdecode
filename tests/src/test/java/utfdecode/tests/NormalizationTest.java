package utfdecode.tests;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import java.text.Normalizer;
import java.util.List;

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

    @Test void orderingOfCombiners() {
        for (var suffix : List.of("a", "")) {
            var s = "q\u0307\u0323" + suffix;
            for (var form : Normalizer.Form.values()) {
                var normalizedByJava = Normalizer.normalize(s, form);
                Assertions.assertEquals("q\u0323\u0307" + suffix, normalizedByJava);

                var normalizedByUtfDecode = Utfdecode.getUtf8Output(s, form);
                Assertions.assertEquals(normalizedByJava, normalizedByUtfDecode);
            }
        }
    }

}
