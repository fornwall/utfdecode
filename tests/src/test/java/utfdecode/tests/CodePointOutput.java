package utfdecode.tests;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

class CodePointOutput {

    @Test void codePointOutput() {
        var output = Utfdecode.getUtf8Output("a");
        Assertions.assertEquals("'a' = U+0061 (LATIN SMALL LETTER A)\n", output);

        output = Utfdecode.getUtf8Output("ö");
        Assertions.assertEquals(output, "'ö' = U+00F6 (LATIN SMALL LETTER O WITH DIAERESIS)\n");
    }

    @Test void surrogates() {
        var nameFromJava = Character.getName(0xDC00);
        Assertions.assertEquals("LOW SURROGATES DC00", nameFromJava);

        nameFromJava = Character.getName(0xDC01);
        Assertions.assertEquals("LOW SURROGATES DC01", nameFromJava);
    }

    @Test void privateUse() {
        var nameFromJava = Character.getName(0xE000);
        Assertions.assertEquals("PRIVATE USE AREA E000", nameFromJava);
        var nameFromUtfDecode = Utfdecode.getUtf8Output("\uE000");
        Assertions.assertEquals(nameFromUtfDecode, "'\uE000' = U+E000 (PRIVATE USE AREA E000)\n");

        nameFromJava = Character.getName(0xE001);
        Assertions.assertEquals("PRIVATE USE AREA E001", nameFromJava);
        nameFromUtfDecode = Utfdecode.getUtf8Output("\uE001");
        Assertions.assertEquals(nameFromUtfDecode, "'\uE001' = U+E000 (PRIVATE USE AREA E001)\n");
    }

}
