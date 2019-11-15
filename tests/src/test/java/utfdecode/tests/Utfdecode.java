package utfdecode.tests;

import com.google.common.io.CharStreams;

import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.nio.charset.Charset;
import java.nio.charset.StandardCharsets;
import java.text.Normalizer;

public class Utfdecode {

    public static String getUtf8Output(String input) {
        try {
            var process = Runtime.getRuntime().exec("../build/utfdecode");
            try (var out = new OutputStreamWriter(process.getOutputStream(), StandardCharsets.UTF_8)) {
                out.write(input);
            }
            return CharStreams.toString(new InputStreamReader(process.getInputStream(), StandardCharsets.UTF_8));
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    public static String getUtf8Output(String input, Normalizer.Form form) {
        try {
            var process = Runtime.getRuntime().exec("../build/utfdecode -e utf8 -n " + form);
            try (var out = new OutputStreamWriter(process.getOutputStream(), StandardCharsets.UTF_8)) {
                out.write(input);
            }
            return CharStreams.toString(new InputStreamReader(process.getInputStream(), StandardCharsets.UTF_8));
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    private static String utfdecodeCharsetName(Charset encoding) {
        if (encoding == StandardCharsets.UTF_8) {
            return "utf8";
        } else if (encoding == StandardCharsets.UTF_16BE) {
            return "utf16be";
        } else if (encoding == StandardCharsets.UTF_16LE) {
            return "utf16le";
        } else {
            throw new IllegalArgumentException("Unsupported charset: " + encoding);
        }
    }

    public static String getOutput(String input, Charset decodingFrom, Charset encodingTo) {
        try {
            var process = Runtime.getRuntime().exec("../build/utfdecode"
                    + " -d " + utfdecodeCharsetName(decodingFrom)
                    + " -e " + utfdecodeCharsetName(encodingTo));
            try (var out = new OutputStreamWriter(process.getOutputStream(), decodingFrom)) {
                out.write(input);
            }
            return CharStreams.toString(new InputStreamReader(process.getInputStream(), encodingTo));
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }
}
