package utfdecode.tests;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import java.text.Normalizer;
import java.util.List;
import java.util.Map;

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
        for (var input : List.of("q\u0307\u0323", "q\u0323\u0307")) {
            for (var suffix : List.of("a", "")) {
                var s = input + suffix;
                for (var form : Normalizer.Form.values()) {
                    var normalizedByJava = Normalizer.normalize(s, form);
                    Assertions.assertEquals("q\u0323\u0307" + suffix, normalizedByJava);
                    var normalizedByUtfDecode = Utfdecode.getUtf8Output(s, form);
                    Assertions.assertEquals(normalizedByJava, normalizedByUtfDecode);
                }
            }
        }
    }

    @Test void singletonEquivalence() {
        for (var input : List.of("\u2126", "\u03A9")) {
            for (var form : Normalizer.Form.values()) {
                var normalizedByJava = Normalizer.normalize(input, form);
                Assertions.assertEquals("\u03A9", normalizedByJava);
                var normalizedByUtfDecode = Utfdecode.getUtf8Output(input, form);
                Assertions.assertEquals(normalizedByJava, normalizedByUtfDecode);
            }
        }
    }

    @Test void decompose1EBF() {
        var s = "\u1EBF";
        for (var form : List.of(Normalizer.Form.NFD, Normalizer.Form.NFKD)) {
            var normalizedByJava = Normalizer.normalize(s, form);
            Assertions.assertEquals("e\u0302\u0301", normalizedByJava);
            var normalizedByUtfDecode = Utfdecode.getUtf8Output(s, form);
            Assertions.assertEquals(normalizedByJava, normalizedByUtfDecode);
        }
    }

    @Test void decompose00C7() {
        var s = "Ç";
        for (var form : List.of(Normalizer.Form.NFD, Normalizer.Form.NFKD)) {
            var normalizedByJava = Normalizer.normalize(s, form);
            normalizedByJava.codePoints().forEach(c -> {
                System.out.println("U+" + Integer.toHexString(c));
            });
            Assertions.assertEquals("C\u0327", normalizedByJava);

            var normalizedByUtfDecode = Utfdecode.getUtf8Output(s, form);
            Assertions.assertEquals(normalizedByJava, normalizedByUtfDecode);
        }
    }

    @Test void compatible() {
        for (var input : Map.of(
                "ℌ", "H",
                "ℍ", "H",
                "①", "1",
                "ｶ", "カ",
                "︷", "{",
                "︸", "}",
                "i⁹", "i9",
                "¼", "1\u20444"
        ).entrySet()) {
            for (var form : List.of(Normalizer.Form.NFKC, Normalizer.Form.NFKD)) {
                var normalizedByJava = Normalizer.normalize(input.getKey(), form);
                Assertions.assertEquals(input.getValue(), normalizedByJava);
                var normalizedByUtfDecode = Utfdecode.getUtf8Output(input.getKey(), form);
                Assertions.assertEquals(input.getValue(), normalizedByUtfDecode);
            }
            for (var form : List.of(Normalizer.Form.NFC, Normalizer.Form.NFD)) {
                var normalizedByJava = Normalizer.normalize(input.getKey(), form);
                Assertions.assertEquals(input.getKey(), normalizedByJava);
                var normalizedByUtfDecode = Utfdecode.getUtf8Output(input.getKey(), form);
                Assertions.assertEquals(input.getKey(), normalizedByUtfDecode);
            }
        }
    }

    @Test void decomposeHangul() {
        var s = "\uCE31";
        for (var form : List.of(Normalizer.Form.NFD, Normalizer.Form.NFKD)) {
            var normalizedByJava = Normalizer.normalize(s, form);
            Assertions.assertEquals("\u110E\u1173\u11B8", normalizedByJava);
            var normalizedByUtfDecode = Utfdecode.getUtf8Output(s, form);
            Assertions.assertEquals(normalizedByJava, normalizedByUtfDecode);
        }
    }

}
