package utfdecode.tests;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import java.nio.charset.StandardCharsets;
import java.util.List;

class Utf16Tests {

    @Test void codePointOutput() {
        var charsets = List.of(StandardCharsets.UTF_8, StandardCharsets.UTF_16BE, StandardCharsets.UTF_16LE);
        for (var input : List.of("a", "ö", "§", "\uD83D\uDCA9\n")) {
            for (var from : charsets) {
                for (var to : charsets) {
                    var output = Utfdecode.getOutput(input, from, to);
                    Assertions.assertEquals(input, output);
                }
            }
        }
    }

}
