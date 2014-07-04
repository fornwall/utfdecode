#define __STDC_FORMAT_MACROS
#define _XOPEN_SOURCE

#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <locale.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>
#include <wchar.h>

#include "musl/wcwidth_musl.h"
#include "unicode_blocks.hpp"

/* From sysexits.h: */
#define EX_OK           0       /* successful termination */
#define EX_USAGE        64      /* command line usage error */
#define EX_DATAERR      65      /* data format error */
#define EX_NOINPUT      66      /* cannot open input */
#define EX_SOFTWARE     70      /* internal software error */
#define EX_IOERR        74      /* input/output error */
#define EX_NOPERM       77      /* permission denied */

enum class input_format_t { UTF8, UTF16BE, UTF16LE, UTF32LE, UTF32BE, TEXTUAL_CODEPOINT };
enum class output_format_t { UTF8, UTF16BE, UTF16LE, UTF32LE, UTF32BE, DESCRIPTION_CODEPOINT, DESCRIPTION_DECODING, SILENT };
enum class error_handling_t { ABORT, REPLACE, IGNORE };
enum class error_reporting_t { REPORT_STDERR, SILENT };

void die_with_internal_error[[noreturn]](char const* fmt, ...){
        fprintf(stderr, "utfdecode: internal error - ");
        va_list argp;
        va_start(argp, fmt);
        vfprintf(stderr, fmt, argp);
        va_end(argp);
        fprintf(stderr, "\n");
        exit(EX_SOFTWARE);
}

struct program_options_t {
        input_format_t input_format{input_format_t::UTF8};
        output_format_t output_format{output_format_t::DESCRIPTION_DECODING};
        error_handling_t error_handling{error_handling_t::IGNORE};
        error_reporting_t error_reporting{error_reporting_t::REPORT_STDERR};
        bool newlines_between_reads_for_description{false};
        bool timestamps{false};
        bool print_summary{false};
        bool terminal_color_output{false};
        bool input_is_terminal{false};

        struct termios vt_orig;

        uint64_t byte_skip_offset{0};
        uint64_t byte_skip_limit{0};
        uint64_t bytes_into_input{0};
        uint64_t codepoints_into_input{0};
        uint64_t error_count{0};

        bool print_byte_input() const { return output_format == output_format_t::DESCRIPTION_DECODING && input_format != input_format_t::TEXTUAL_CODEPOINT; };
        bool is_silent_output() const { return output_format == output_format_t::SILENT; };
        // bool is_report_errors() const { return error_reporting == error_reporting_t::REPORT_STDERR; }
        //void check_exit_on_error() { if (error_handling == error_handling_t::ABORT) exit(EX_DATAERR); }
        bool is_replace_errors() { return error_handling == error_handling_t::REPLACE; }

        void output_formatted_byte(FILE* where, uint8_t byte) {
                fprintf(where, "0x%02X - ", (int) byte);
        }


        void note_error(int byte, char const* error_msg, ...) {
                error_count++;
                if (error_reporting == error_reporting_t::REPORT_STDERR) {
                        fflush(stdout);
                        char const* color_prefix = terminal_color_output ? "\x1B[31m" : "";
                        char const* color_suffix = terminal_color_output ? "\x1B[m" : "";
                        fprintf(stderr, "%s", color_prefix);
                        output_formatted_byte(stderr, byte);
                        fprintf(stderr, "malformed %" PRIu64 " %s and %" PRIu64 " %s in - ",
                                        bytes_into_input, (bytes_into_input == 1 ? "byte" : "bytes"),
                                        codepoints_into_input, codepoints_into_input == 1 ? "character" : "characters");
                        va_list argp;
                        va_start(argp, error_msg);
                        vfprintf(stderr, error_msg, argp);
                        va_end(argp);

                        fprintf(stderr, "%s\n", color_suffix);
                        fflush(stderr);
                } else {
                        if (print_byte_input()) output_formatted_byte(stdout, byte);
                }

                if (error_handling == error_handling_t::ABORT) {
                        exit(EX_DATAERR);
                }
        }
        
        void cleanup_and_exit(int exit_status) {
                if (input_is_terminal) tcsetattr(0, TCSANOW, &vt_orig);
                exit(exit_status);
        }

        void encode_utf16(uint16_t codeunit, uint8_t* buffer) {
                bool little_endian = (output_format == output_format_t::UTF16LE);
                buffer[0] = little_endian ? (codeunit & 0xFF) : (codeunit >> 8);
                buffer[1] = little_endian ? (codeunit >> 8) : (codeunit & 0xFF);
                buffer[2] = 0;
        }

        void print_byte_result(int byte, char const* msg, ...) {
                if (print_byte_input()) {
                        output_formatted_byte(stdout, byte);
                        va_list argp;
                        va_start(argp, msg);
                        vfprintf(stdout, msg, argp);
                        va_end(argp);
                }
        }
};

/* Assumes that the buffer is already validated. Does not handle overlong encodings. */
uint32_t utf8_sequence_to_codepoint(uint8_t* buffer, uint8_t length) {  
        uint32_t code_point;
        uint32_t first_byte_mask;
        switch (length) {
                case 2: first_byte_mask = 0x1F; break;
                case 3: first_byte_mask = 0x0F; break;
                case 4: first_byte_mask = 0x07; break;
                default: die_with_internal_error("utf8_sequence_to_codepoint(): length=%d\n", (int) length);
        }
        code_point = (buffer[0] & first_byte_mask);
        for (int i = 1; i < length; i++) code_point = ((code_point << 6) | (uint32_t) (buffer[i] & 0x3F));
        return code_point; 
}

int codepoint_to_utf8(uint32_t codePoint, uint8_t* utf8InputBuffer) {
        int bufferPosition = 0;
        if (codePoint <= /* 7 bits */0b1111111) {
                utf8InputBuffer[bufferPosition++] = (uint8_t) codePoint;
        } else if (codePoint <= /* 11 bits */0b11111111111) {
                /* 110xxxxx leading byte with leading 5 bits */
                utf8InputBuffer[bufferPosition++] = (uint8_t) (0b11000000 | (codePoint >> 6));
                /* 10xxxxxx continuation byte with following 6 bits */
                utf8InputBuffer[bufferPosition++] = (uint8_t) (0b10000000 | (codePoint & 0b111111));
        } else if (codePoint <= /* 16 bits */0b1111111111111111) {
                /* 1110xxxx leading byte with leading 4 bits */
                utf8InputBuffer[bufferPosition++] = (uint8_t) (0b11100000 | (codePoint >> 12));
                /* 10xxxxxx continuation byte with following 6 bits */
                utf8InputBuffer[bufferPosition++] = (uint8_t) (0b10000000 | ((codePoint >> 6) & 0b111111));
                /* 10xxxxxx continuation byte with following 6 bits */
                utf8InputBuffer[bufferPosition++] = (uint8_t) (0b10000000 | (codePoint & 0b111111));
        } else if (codePoint <= /* 21 bits */0b111111111111111111111) {
                /* 11110xxx leading byte with leading 3 bits */
                utf8InputBuffer[bufferPosition++] = (uint8_t) (0b11110000 | (codePoint >> 18));
                /* 10xxxxxx continuation byte with following 6 bits */
                utf8InputBuffer[bufferPosition++] = (uint8_t) (0b10000000 | ((codePoint >> 12) & 0b111111));
                /* 10xxxxxx continuation byte with following 6 bits */
                utf8InputBuffer[bufferPosition++] = (uint8_t) (0b10000000 | ((codePoint >> 6) & 0b111111));
                /* 10xxxxxx continuation byte with following 6 bits */
                utf8InputBuffer[bufferPosition++] = (uint8_t) (0b10000000 | (codePoint & 0b111111));
        } else {
                die_with_internal_error("codepoint_to_utf8(): invalid code point %d", codePoint);
        }
        return bufferPosition;
}

void encode_codepoint(uint32_t codepoint, program_options_t& program) {
        program.codepoints_into_input++;
        if (program.is_silent_output()) return;

        if (program.output_format == output_format_t::DESCRIPTION_DECODING || program.output_format == output_format_t::DESCRIPTION_CODEPOINT) {
                printf("U+%04X", codepoint);
                uint8_t utf8_buffer[5];
                int utf8_byte_count = codepoint_to_utf8(codepoint, utf8_buffer);
                utf8_buffer[utf8_byte_count] = 0;
                int wcwidth_value = wcwidth_musl(codepoint);
                if (wcwidth_value != -1) {
                        printf(" = '%s'", utf8_buffer);
                }
                if (codepoint <= 127 && program.output_format == output_format_t::DESCRIPTION_DECODING) {
                        switch (codepoint) {
                                case 0:   printf(" (Null character ^@ \\0)"); break;
                                case 1:   printf(" (Start of Header ^A)"); break;
                                case 2:   printf(" (Start of Text ^B)"); break;
                                case 3:   printf(" (End of Text	^C)"); break;
                                case 4:   printf(" (End of Transmission	^D)"); break;
                                case 5:   printf(" (Enquiry ^E)"); break;
                                case 6:   printf(" (Acknowledgement ^F)"); break;
                                case 7:   printf(" (Bell ^G)"); break;
                                case 8:   printf(" (Backspace ^H \\b)"); break;
                                case 9:   printf(" (Horizontal tab ^I \\t)"); break;
                                case 10:  printf(" (Line feed (^J, \\n)"); break;
                                case 11:  printf(" (Vertical tab ^K \\v)"); break;
                                case 12:  printf(" (Form feed ^L \\f)"); break;
                                case 13:  printf(" (Carriage return (^M, \\r)"); break;
                                case 14:  printf(" (Start of Text ^N)"); break;
                                case 15:  printf(" (Shift In ^O	)"); break;
                                case 16:  printf(" (Shift Out ^P )"); break;
                                case 17:  printf(" (Device Control 1 ^Q)"); break;
                                case 18:  printf(" (Device Control 2 ^R)"); break;
                                case 19:  printf(" (Device Control 3 ^S)"); break;
                                case 20:  printf(" (Device Control 4 ^T)"); break;
                                case 21:  printf(" (Negative Acknowledgement ^U)"); break;
                                case 22:  printf(" (Synchronous idle ^V)"); break;
                                case 23:  printf(" (End of Transmission Block ^W)"); break;
                                case 24:  printf(" (Cancel ^X)"); break;
                                case 25:  printf(" (End of Medium ^Y)"); break;
                                case 26:  printf(" (Substitute ^Z)"); break;
                                case 27:  printf(" (Escape ^[ \\e)"); break;
                                case 28:  printf(" (File Separator ^\\)"); break;
                                case 29:  printf(" (Record Separator ^])"); break;
                                case 30:  printf(" (Group Separator ^^)"); break;
                                case 31:  printf(" (Unit Separator ^_)"); break;
                                case 127: printf(" (Delete ^?)"); break;
                        }
                }
                if (program.output_format == output_format_t::DESCRIPTION_DECODING) {
                        char const* plane_name = "???";
                        if (codepoint <= 0xFFFF) {
                                plane_name = "0 Basic Multilingual Plane (BMP)";
                        } else if (codepoint <= 0x1FFFF) {
                                plane_name = "1 Supplementary Multilingual Plane (SMP)";
                        } else if (codepoint < 0x2FFFF) {
                                plane_name = "2 Supplementary Ideographic Plane (SIP)";
                        } else if (codepoint <= 0xDFFFF) {
                                plane_name = "*Unassigned*";
                        } else if (codepoint <= 0xEFFFF) {
                                plane_name = "14 Supplement­ary Special-purpose Plane (SSP)";
                        } else if (codepoint <= 0xFFFFD) {
                                plane_name = "15 Supplemental Private Use Area-A (S PUA A)";
                        } else if (codepoint <= 0x10FFFD) {
                                plane_name = "16 Supplemental Private Use Area-B (S PUA B)";
                        } else {
                                die_with_internal_error("plane out of range");
                        }

                        char const* block_name = get_block_name(codepoint);
                        printf(" from the block %s in plane %s. wcwidth=%d\n", block_name, plane_name, wcwidth_value);
                } else {
                        printf("\n");
                }
        } else if (program.output_format == output_format_t::UTF8) {
                uint8_t utf8_buffer[5];
                int utf8_byte_count = codepoint_to_utf8(codepoint, utf8_buffer);
                utf8_buffer[utf8_byte_count] = 0;
                printf("%s", utf8_buffer);
                fflush(stdout);
        } else if (program.output_format == output_format_t::UTF16BE || program.output_format == output_format_t::UTF16LE) {
                uint8_t output[5];
                int output_length = 2;
                if (codepoint <= 0xFFFF) {
                        program.encode_utf16(codepoint, output);
                } else {
                        // Code points from the other planes (called Supplementary Planes) are encoded in UTF-16 by pairs of
                        // 16-bit code units called surrogate pairs, by the following scheme:
                        // (1) 0x010000 is subtracted from the code point, leaving a 20 bit number in the range 0..0x0FFFFF.
                        // (2) The top ten bits (a number in the range 0..0x03FF) are added to 0xD800 to give the first code
                        // unit or lead surrogate, which will be in the range 0xD800..0xDBFF.
                        // (3) The low ten bits (also in the range 0..0x03FF) are added to 0xDC00 to give the second code unit
                        // or trail surrogate, which will be in the range 0xDC00..0xDFFF.
                        codepoint -= 0x010000;
                        uint16_t first = (codepoint >> 10) + 0xD800;
                        uint16_t second = (0b1111111111 & codepoint) + 0xDC00;
                        program.encode_utf16(first, output);
                        program.encode_utf16(second, output + 2);
                        output_length = 4;
                }
                write(STDOUT_FILENO, output, output_length);
                fflush(stdout);
        } else if (program.output_format == output_format_t::UTF32BE || program.output_format == output_format_t::UTF32LE) {
                uint8_t buffer[5];
                bool little_endian = program.output_format == output_format_t::UTF32LE;
                for (int i = 0; i < 4; i++) {
                        int shift = 8 * (little_endian ? i : (3 - i));
                        buffer[i] = (codepoint >> shift) & 0xFF;
                }
                write(STDOUT_FILENO, buffer, 4);
                fflush(stdout);
        }
}

void process_utf8_byte(uint8_t byte, uint8_t* utf8_buffer, uint8_t& utf8_pos, uint8_t& remaining_utf8_continuation_bytes, program_options_t& options) {
        bool invalid_utf8_seq = false; 
        if (options.input_format == input_format_t::UTF16LE) {
                return;
        }
        if (byte <= 127) {
                invalid_utf8_seq = (remaining_utf8_continuation_bytes > 0);
                if (invalid_utf8_seq) {
                        options.note_error(byte, "expected continuation byte");
                } else {
                        options.print_byte_result(byte, "single-byte UTF-8 sequence: ");
                        encode_codepoint(byte, options);
                }
        } else if ((byte & /*0b11000000=*/0xc0) == /*0b10000000=*/0x80) {
                invalid_utf8_seq = (remaining_utf8_continuation_bytes == 0);
                if (!invalid_utf8_seq) {
                        utf8_buffer[utf8_pos++] = byte;
                        --remaining_utf8_continuation_bytes;
                        if (remaining_utf8_continuation_bytes == 0) {
                                uint8_t used_length = utf8_pos;

                                utf8_buffer[used_length] = 0;
                                uint32_t code_point = utf8_sequence_to_codepoint(utf8_buffer, used_length);
                                if (code_point > 0x10FFFF) {
                                        options.note_error(byte, "code point out of range: %u", code_point);
                                } else if (((code_point <= 0x80) && used_length > 1)
                                                || (code_point < 0x800 && used_length > 2)
                                                || (code_point < 0x10000 && used_length > 3)) {
                                        options.note_error(byte, "overlong encoding of %u using %d bytes", code_point, used_length);
                                } else {
                                        options.print_byte_result(byte, "UTF-8 continuation byte 0b10xxxxxx: ");
                                        encode_codepoint(code_point, options);
                                }
                                remaining_utf8_continuation_bytes = 0;
                        } else {
                                options.print_byte_result(byte, "UTF-8 continuation byte 0b10xxxxxx\n");
                        }
                }
        } else {
                int expect_following;
                if        ((byte & /*0b11100000=*/0xe0) == /*0b11000000=*/0xc0) {
                        expect_following = 1;
                } else if ((byte & /*0b11110000=*/0xf0) == /*0b11100000=*/0xe0) {
                        expect_following = 2;
                } else if ((byte & /*0b11111000=*/0xf8) == /*0b11110000=*/0xf0) {
                        expect_following = 3;
                } else {
                        expect_following = -1;
                        invalid_utf8_seq = true;
                        options.note_error(byte, "invalid UTF-8 byte");
                }
                if (expect_following != -1) {
                        if (remaining_utf8_continuation_bytes == 0) {
                                options.print_byte_result(byte,
                                                "leading %s byte for a %d byte sequence\n",
                                                (expect_following == 1) ? "0b110xxxxx" :
                                                ((expect_following == 2) ? "0b1110xxxx" : "0b11110xxx"),
                                                expect_following + 1);
                                remaining_utf8_continuation_bytes = expect_following;
                                utf8_pos = 1;
                                utf8_buffer[0] = byte;
                        } else {
                                invalid_utf8_seq = true;
                        }
                }
        }

        // FIXME: https://github.com/jackpal/Android-Terminal-Emulator/commit/7335c643f758ce4351ac00813027ce1505f8dbd4
        // - should continue with the current byte in the buffer
        if (invalid_utf8_seq) {
                remaining_utf8_continuation_bytes = 0;
        }
}

void process_utf16_byte(uint8_t byte, uint8_t* state_buffer, uint8_t& state_pos, program_options_t& options) {
        state_buffer[state_pos++] = byte;
        if (state_pos == 2) {
                uint16_t codeuint = (options.input_format == input_format_t::UTF16LE)
                        ? state_buffer[0] + (uint16_t(state_buffer[1]) << 8)
                        : state_buffer[1] + (uint16_t(state_buffer[0]) << 8);
                if (codeuint >= 0xD800 && codeuint <= 0xDBFF) {
                        options.print_byte_result(byte, "leading surrogate %d\n", codeuint);
                } else if (codeuint >= 0xDC00 && codeuint <= 0xDFFF) {
                        options.note_error(byte, "trailing surrogate %d without leading surrogate before", codeuint);
                        state_pos = 0;
                } else {
                        options.print_byte_result(byte, "single UTF-16 code unit: ");
                        encode_codepoint(codeuint, options);
                        state_pos = 0;
                }
        } else if (state_pos == 4) {
                uint16_t leading_surrogate = (options.input_format == input_format_t::UTF16LE)
                        ? state_buffer[0] + (uint16_t(state_buffer[1]) << 8)
                        : state_buffer[1] + (uint16_t(state_buffer[0]) << 8);
                uint16_t trailing_surrogate = (options.input_format == input_format_t::UTF16LE)
                        ? state_buffer[2] + (uint16_t(state_buffer[3]) << 8)
                        : state_buffer[3] + (uint16_t(state_buffer[2]) << 8);
                uint32_t codepoint = 0x010000 + (uint32_t(leading_surrogate - 0xD800) << 10) + (trailing_surrogate - 0xDC00);
                options.print_byte_result(byte, "trailling surrogate %d", trailing_surrogate);
                encode_codepoint(codepoint, options);
                state_pos = 0;
        } else {
                options.print_byte_result(byte, "start of UTF-16 code unit\n");
        }
}

void process_utf32_byte(uint8_t byte, uint8_t* state_buffer, uint8_t& state_pos, program_options_t& options) {
        state_buffer[state_pos++] = byte;
        if (state_pos == 4) {
                uint32_t codepoint = 0;
                for (int i = 0; i < 4; i++) {
                        int shift = 8 * ((options.input_format == input_format_t::UTF32LE) ? i : (3 - i));
                        codepoint += (state_buffer[i] << shift) & (0xFF << shift);
                }
                options.print_byte_result(byte, "byte 4 of a UTF-32 code unit: ");
                encode_codepoint(codepoint, options);
                state_pos = 0;
        } else {
                options.print_byte_result(byte, "byte %d of a UTF-32 code unit\n", state_pos);
        }
}

void process_textual_codepoint_byte(uint8_t byte, uint8_t* state_buffer, uint8_t& state_buffer_pos, program_options_t& options) {
        if (byte == '\n' || byte == ' ' || byte == '\r' || byte == '\t') {
                if (state_buffer_pos > 0) {
                        if (state_buffer_pos > 2 && state_buffer[0] == 'U' && state_buffer[1] == '+') {
                                // Handle "U+XXXX" as "0xXXXX"
                                state_buffer[0] = '0';
                                state_buffer[1] = 'x';
                        }
                        state_buffer[state_buffer_pos] = 0;
                        long codepoint = strtol((char*)state_buffer, NULL, 0);
                        if (codepoint == 0) {
                                options.note_error(byte, "cannot parse into code point: '%s'", state_buffer);
                        } else {
                                encode_codepoint(codepoint, options);
                        }
                        state_buffer_pos = 0;
                }
        } else if (state_buffer_pos >= 16) {
                state_buffer[state_buffer_pos] = 0;
                options.note_error(byte, "too large string '%s' - discarding", state_buffer);
        } else {
                state_buffer[state_buffer_pos++] = byte;
        }

}

void read_and_echo(program_options_t& options) {
        uint8_t remaining_bytes = 0;
        uint8_t state_buffer_position = 0;
        uint8_t state_buffer[16];

        int64_t initial_timestamp = -1;
        if (options.timestamps) {
                struct timeval tv;
                gettimeofday(&tv, NULL);
                initial_timestamp = tv.tv_sec*1000 + tv.tv_usec/1000;
        }

        while (true) {
                unsigned char read_buffer[256];
                int read_now = read(0, read_buffer, sizeof(read_buffer) - 1);
                if (read_now == 0) {
                        return;
                } else if (read_now < 0) {
                        perror("read()");
                        return;
                }


                unsigned char* start_of_buffer = read_buffer;
                if (read_now + options.bytes_into_input < options.byte_skip_offset) {
                        options.bytes_into_input += read_now;
                        continue;
                } else if (options.bytes_into_input < options.byte_skip_offset) {
                        int skipping_now = options.byte_skip_offset - options.bytes_into_input;
                        options.bytes_into_input += skipping_now;
                        read_now -= skipping_now;
                        start_of_buffer = &read_buffer[skipping_now];
                }

                if (options.timestamps) {
                        struct timeval tv;
                        gettimeofday(&tv, NULL);
                        long long int elapsed = (tv.tv_sec*1000 + tv.tv_usec/1000) - initial_timestamp;
                        printf("%lld ms\n", elapsed);
                }

                bool end_of_input = false;
                for (int i = 0; i < read_now; i++) {
                        uint8_t c = start_of_buffer[i];
                        if (options.input_is_terminal && (c == 3 || c == 4)) {
                                /* Let the user exit on ctrl+c or ctrl+d, or on end of file. */
                                end_of_input = true;
                                break;
                        }
                        switch (options.input_format) {
                                case input_format_t::UTF8:
                                        process_utf8_byte(c, state_buffer, state_buffer_position, remaining_bytes, options);
                                        break;
                                case input_format_t::UTF16BE:
                                case input_format_t::UTF16LE:
                                        process_utf16_byte(c, state_buffer, state_buffer_position, options);
                                        break;
                                case input_format_t::UTF32BE:
                                case input_format_t::UTF32LE:
                                        process_utf32_byte(c, state_buffer, state_buffer_position, options);
                                        break;
                                case input_format_t::TEXTUAL_CODEPOINT:
                                        process_textual_codepoint_byte(c, state_buffer, state_buffer_position, options);
                                        break;
                        }
                        options.bytes_into_input++;
                        if (options.byte_skip_limit != 0 && ((options.byte_skip_limit + options.byte_skip_offset) <= options.bytes_into_input)) {
                                end_of_input = true;
                                break;
                        }
                }
                if (options.newlines_between_reads_for_description) printf("\n");
                fflush(stdout);
                if (end_of_input) return;
        }
}

void print_usage_and_exit(char const* program_name, int exit_status) {
        fprintf(stderr, "usage: %s [OPTIONS] [file]\n"
                        "  -d, --decode-format FORMAT     Determine how input should be decoded:\n"
                        "                                     * codepoint - decode input as textual U+XXXX descriptions\n"
                        "                                     * utf8 (default) - decode input as UTF-8\n"
                        "                                     * utf16le - decode input as UTF-8\n"
                        "                                     * utf16be - decode input as UTF-8\n"
                        "                                     * utf32le - decode input as UTF-8\n"
                        "                                     * utf32be - decode input as UTF-8\n"
                        "  -e, --encode-format FORMAT      Determine how output should encoded. Accepts same as the above decoding formats and adds:\n"
                        "                                     * decoding (default) - debug output of the complete decoding process\n"
                        "                                     * silent - no output\n"
                        "  -h, --help                     Show this help and exit\n"
                        "  -l, --limit LIMIT              Only decode up to the specified amount of bytes\n"
                        "  -m, --malformed-handling       Determine what should happen on decoding error:\n"
                        "                                     * ignore - ignore invalid input\n"
                        "                                     * replace (default) - replace with the unicode replacement character � (U+FFFD)\n"
                        "                                     * abort - abort the program directly with exit value 65\n"
                        "  -n, --newlines-after-reads     Write out newlines after each read call\n"
                        "  -o, --offset OFFSET            Skip the specified amount of bytes before starting decoding\n"
                        "  -q, --quiet-errors             Do not log decoding errors to stderr\n"
                        "  -s, --summary                  Show a summary at end of input\n"
                        "  -t, --timestamps               Show a timestamp after each input read\n"
                        , program_name);
        exit(exit_status);
}

int main(int argc, char** argv) {
        setlocale(LC_ALL, NULL);
        program_options_t options;
        struct option getopt_options[] = {
                {"decode-format", required_argument, nullptr, 'd'},
                {"encode-format", required_argument, nullptr, 'e'},
                {"help", no_argument, nullptr, 'h'},
                {"limit", required_argument, nullptr, 'l'},
                {"malformed-handling", required_argument, nullptr, 'm'},
                {"newlines-after-reads", no_argument, nullptr, 'n'},
                {"offset", required_argument, nullptr, 'o'},
                {"quiet-errors", no_argument, nullptr, 'q'},
                {"summary", no_argument, nullptr, 's'},
                {"timestamps", no_argument, nullptr, 't'},
                {0, 0, 0, 0}
        };

        while (true) {
                int option_index = 0;
                int c = getopt_long(argc, argv, "d:e:hl:m:no:qst", getopt_options, &option_index);
                if (c == -1) break;
                bool print_error_and_exit = false;
                int exit_status = EX_USAGE;
                switch (c) {
                        case 'd':
                                if (strcmp(optarg, "utf8") == 0) {
                                        options.input_format = input_format_t::UTF8;
                                } else if (strcmp(optarg, "utf16le") == 0 || strcmp(optarg, "utf16-le") == 0) {
                                        options.input_format = input_format_t::UTF16LE;
                                } else if (strcmp(optarg, "utf16be") == 0 || strcmp(optarg, "utf16-be") == 0) {
                                        options.input_format = input_format_t::UTF16BE;
                                } else if (strcmp(optarg, "utf32le") == 0 || strcmp(optarg, "utf32-le") == 0) {
                                        options.input_format = input_format_t::UTF32LE;
                                } else if (strcmp(optarg, "utf32be") == 0 || strcmp(optarg, "utf32-be") == 0) {
                                        options.input_format = input_format_t::UTF32BE;
                                } else if (strcmp(optarg, "codepoint") == 0) {
                                        options.input_format = input_format_t::TEXTUAL_CODEPOINT;
                                } else {
                                        fprintf(stderr, "'%s' is not a valid decode format\n", optarg);
                                        print_error_and_exit = true;
                                }
                                break;
                        case 'e':
                                if (strcmp(optarg, "codepoint") == 0) {
                                        options.output_format = output_format_t::DESCRIPTION_CODEPOINT;
                                } else if (strcmp(optarg, "decoding") == 0) {
                                        options.output_format = output_format_t::DESCRIPTION_DECODING;
                                } else if (strcmp(optarg, "utf8") == 0) {
                                        options.output_format = output_format_t::UTF8;
                                } else if (strcmp(optarg, "utf16le") == 0 || strcmp(optarg, "utf16-le") == 0) {
                                        options.output_format = output_format_t::UTF16LE;
                                } else if (strcmp(optarg, "utf16be") == 0 || strcmp(optarg, "utf16-be") == 0) {
                                        options.output_format = output_format_t::UTF16BE;
                                } else if (strcmp(optarg, "utf32le") == 0 || strcmp(optarg, "utf32-le") == 0) {
                                        options.output_format = output_format_t::UTF32LE;
                                } else if (strcmp(optarg, "utf32be") == 0 || strcmp(optarg, "utf32-be") == 0) {
                                        options.output_format = output_format_t::UTF32BE;
                                } else if (strcmp(optarg, "silent") == 0) {
                                        options.output_format = output_format_t::SILENT;
                                } else {
                                        fprintf(stderr, "'%s' is not a valid encode format\n", optarg);
                                        print_error_and_exit = true;
                                }
                                break;
                        case 'h':
                                exit_status = EX_OK;
                                print_error_and_exit = true;
                                break;
                        case 'l':
                                options.byte_skip_limit = atoi(optarg);
                                if (options.byte_skip_limit == 0) {
                                        fprintf(stderr, "'%s' is not a valid byte limit\n", optarg);
                                        print_error_and_exit = true;
                                }
                                break;
                        case 'm':
                                if (strcmp(optarg, "ignore") == 0) {
                                        options.error_handling = error_handling_t::IGNORE;
                                } else if (strcmp(optarg, "replace") == 0) {
                                        options.error_handling = error_handling_t::REPLACE;;
                                } else if (strcmp(optarg, "abort") == 0) {
                                        options.error_handling = error_handling_t::ABORT;
                                } else {
                                        fprintf(stderr, "'%s' is not a valid error handling\n", optarg);
                                        print_error_and_exit = true;
                                }
                                break;
                        case 'n':
                                options.newlines_between_reads_for_description = true;
                                break;
                        case 'o':
                                options.byte_skip_offset = atoi(optarg);
                                if (options.byte_skip_offset == 0) {
                                        fprintf(stderr, "'%s' is not a valid byte offset\n", optarg);
                                        print_error_and_exit = true;
                                }
                                break;
                        case 'q':
                                options.error_reporting = error_reporting_t::SILENT;
                                break;
                        case 's':
                                options.print_summary = true;
                                break;
                        case 't': 
                                options.timestamps = true; 
                                break;
                        default:
                                print_error_and_exit = true;
                                break;
                }
                if (print_error_and_exit) print_usage_and_exit(argv[0], exit_status);
        }

        if (optind + 1 == argc) {
                if (strcmp(argv[optind], "-") != 0) {
                        int file_fd = open(argv[optind], O_RDONLY);
                        if (file_fd < 0) {
                                fprintf(stderr, "%s - ", argv[optind]);
                                perror("");
                                return EX_NOINPUT;
                        }
                        if (dup2(file_fd, STDIN_FILENO)) {
                                perror("utfdecode: dup2()");
                                return EX_IOERR;
                        }
                        close(file_fd);
                }
        } else if (optind < argc) {
                print_usage_and_exit(argv[0], EX_USAGE);
        }

        bool is_output_tty = isatty(STDOUT_FILENO);
        if (is_output_tty) options.terminal_color_output = true;

        // If inputting textual code points, do not special case terminal input:
        options.input_is_terminal = (options.input_format != input_format_t::TEXTUAL_CODEPOINT) && isatty(STDIN_FILENO);

        if (options.input_is_terminal) {
                tcgetattr(0, &options.vt_orig);
                struct termios vt_new = options.vt_orig;
                vt_new.c_cc[VMIN] = 1; 	// Minimum number of characters for noncanonical read (MIN).
                vt_new.c_cc[VTIME] = 0; 	// Timeout in deciseconds for noncanonical read (TIME).
                // echo off, canonical mode off, extended input processing off, signal chars off:
                vt_new.c_lflag &=  IEXTEN | ISIG;

                tcsetattr(STDIN_FILENO, TCSANOW, &vt_new);
        }
        read_and_echo(options);

        if (options.print_summary) {
                char const* color_prefix = options.terminal_color_output ? "\x1B[35m" : "";
                char const* color_suffix = options.terminal_color_output ? "\x1B[m" : "";
                uint64_t bytes_encountered = options.bytes_into_input - options.byte_skip_offset;
                fprintf(stderr, "%sSummary: %" PRIu64 " code points decoded from %" PRIu64 " bytes with %" PRIu64 " errors%s\n",
                                 color_prefix, options.codepoints_into_input, bytes_encountered, options.error_count, color_suffix);
        } else if (is_output_tty && options.output_format != output_format_t::DESCRIPTION_CODEPOINT  && options.output_format != output_format_t::DESCRIPTION_DECODING && options.output_format != output_format_t::SILENT) {
                // Finish with newlines if writing raw output to terminal
                printf("\n");
        }

        int exit_status = options.error_count == 0 ? EX_OK : EX_DATAERR;
        options.cleanup_and_exit(exit_status);
}
