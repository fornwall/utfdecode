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

}
