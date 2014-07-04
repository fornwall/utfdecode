#define __STDC_FORMAT_MACROS

#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

/* From sysexits.h: */
#define EX_OK           0       /* successful termination */
#define EX_USAGE        64      /* command line usage error */
#define EX_DATAERR      65      /* data format error */
#define EX_NOINPUT      66      /* cannot open input */
#define EX_NOUSER       67      /* addressee unknown */
#define EX_NOHOST       68      /* host name unknown */
#define EX_UNAVAILABLE  69      /* service unavailable */
#define EX_SOFTWARE     70      /* internal software error */
#define EX_OSERR        71      /* system error (e.g., can't fork) */
#define EX_OSFILE       72      /* critical OS file missing */
#define EX_CANTCREAT    73      /* can't create (user) output file */
#define EX_IOERR        74      /* input/output error */
#define EX_TEMPFAIL     75      /* temp failure; user is invited to retry */
#define EX_PROTOCOL     76      /* remote error in protocol */
#define EX_NOPERM       77      /* permission denied */
#define EX_CONFIG       78      /* configuration error */

enum class input_format_t { UTF8, UTF16BE, UTF16LE, UTF32BE, UTF32LE, TEXTUAL_CODEPOINT };
enum class output_format_t { UTF8, UTF16BE, UTF16LE, UTF32BE, UTF32LE, DESCRIPTION_CODEPOINT, DESCRIPTION_DECODING, SILENT };
enum class error_handling_t { ABORT, REPLACE, IGNORE };
enum class error_reporting_t { REPORT_STDERR, SILENT };

void die_with_internal_error(char const* msg) {
        fprintf(stderr, "utfdecode fatal internal error - %s\n", msg);
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

        bool print_byte_input() const { return output_format == output_format_t::DESCRIPTION_DECODING; };
        bool is_silent_output() const { return output_format == output_format_t::SILENT; };
        // bool is_report_errors() const { return error_reporting == error_reporting_t::REPORT_STDERR; }
        //void check_exit_on_error() { if (error_handling == error_handling_t::ABORT) exit(EX_DATAERR); }
        bool is_replace_errors() { return error_handling == error_handling_t::REPLACE; }

        void note_error(char const* error_msg) {
                error_count++;
                if (error_reporting == error_reporting_t::REPORT_STDERR) {
                        fflush(stdout);
                        char const* color_prefix = terminal_color_output ? "\x1B[31m" : "";
                        char const* color_suffix = terminal_color_output ? "\x1B[m" : "";
                        fprintf(stderr, "%sutfdecode: decoding error after %" PRIu64 " bytes and %" PRIu64 " code points - %s%s\n", color_prefix, bytes_into_input, codepoints_into_input, error_msg, color_suffix);
                        fflush(stderr);
                }
                if (error_handling == error_handling_t::ABORT) {
                        exit(EX_DATAERR);
                }
        }

        void cleanup_and_exit(int exit_status) {
                if (input_is_terminal) tcsetattr(0, TCSANOW, &vt_orig);
                exit(exit_status);
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
                default: fprintf(stderr, "utfdecode internal error - utf8_sequence_to_codepoint(): length=%d\n", (int) length); exit(EX_SOFTWARE);
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
                fprintf(stderr, "utfdecode internal error - codepoint_to_utf8(): invalid code point %d", codePoint);
                exit(EX_SOFTWARE);
        }
        return bufferPosition;
}

void describe_codepoint(uint32_t codepoint, program_options_t& program) {
        program.codepoints_into_input++;
        if (program.is_silent_output()) return;

        if (program.output_format == output_format_t::DESCRIPTION_DECODING || program.output_format == output_format_t::DESCRIPTION_CODEPOINT) {
                printf("U+%04X", codepoint);
                if (codepoint <= 127 ) {
                        switch (codepoint) {
                                case 0:   printf(" Null character		^@	\\0"); break;
                                case 1:   printf(" Start of Header		^A	 "); break;
                                case 2:   printf(" Start of Text			^B	 "); break;
                                case 3:   printf(" End of Text			^C	 "); break;
                                case 4:   printf(" End of Transmission		^D	 "); break;
                                case 5:   printf(" Enquiry			^E	 "); break;
                                case 6:   printf(" Acknowledgement		^F	 "); break;
                                case 7:   printf(" Bell				^G	 "); break;
                                case 8:   printf(" Backspace			^H	\\b"); break;
                                case 9:   printf(" Horizontal tab		^I	\\t"); break;
                                case 10:  printf(" Line feed (^J, \\n)"); break;
                                case 11:  printf(" Vertical tab			^K	\\v"); break;
                                case 12:  printf(" Form feed			^L	\\f"); break;
                                case 13:  printf(" Carriage return (^M, \\r"); break;
                                case 14:  printf(" Start of Text			^N	  "); break;
                                case 15:  printf(" Shift In			^O	  "); break;
                                case 16:  printf(" Shift Out			^P	  "); break;
                                case 17:  printf(" Device Control 1		^Q	  "); break;
                                case 18:  printf(" Device Control 2		^R	  "); break;
                                case 19:  printf(" Device Control 3		^S	  "); break;
                                case 20:  printf(" Device Control 4		^T	  "); break;
                                case 21:  printf(" Negative Acknowledgement	^U	  "); break;
                                case 22:  printf(" Synchronous idle		^V	  "); break;
                                case 23:  printf(" End of Transmission Block	^W	  "); break;
                                case 24:  printf(" Cancel			^X	  "); break;
                                case 25:  printf(" End of Medium			^Y	  "); break;
                                case 26:  printf(" Substitute			^Z	  "); break;
                                case 27:  printf(" Escape			^[	\\e"); break;
                                case 28:  printf(" File Separator		^\\	  "); break;
                                case 29:  printf(" Record Separator		^]	  "); break;
                                case 30:  printf(" Group Separator		^^	  "); break;
                                case 31:  printf(" Unit Separator		^_	  "); break;
                                case 127: printf(" Delete			^?	  "); break;
                        }
                }
                uint8_t utf8_buffer[5];
                int utf8_byte_count = codepoint_to_utf8(codepoint, utf8_buffer);
                utf8_buffer[utf8_byte_count] = 0;
                bool write_output = (codepoint != '\n');
                if (write_output) {
                        printf(" = '%s'", utf8_buffer);
                }
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

                printf("    Plane:%s\n", plane_name);
        } else if (program.output_format == output_format_t::UTF8) {
                uint8_t utf8_buffer[5];
                int utf8_byte_count = codepoint_to_utf8(codepoint, utf8_buffer);
                utf8_buffer[utf8_byte_count] = 0;
                printf("%s", utf8_buffer);
                fflush(stdout);
        } else if (program.output_format == output_format_t::UTF32LE) {
                uint8_t buffer[5];
                printf("%s", buffer);
                fflush(stdout);
                for (int i = 0; i < 4; i++) buffer[i] = 0b11111111 & (codepoint >> (3-i));
        } else if (program.output_format == output_format_t::UTF32BE) {
                uint8_t buffer[5];
                for (int i = 0; i < 4; i++) buffer[i] = 0b11111111 & (codepoint >> i);
                printf("%s", buffer);
                fflush(stdout);
        }
}

void describe_byte(uint8_t byte, uint8_t* utf8_buffer, uint8_t* utf8_pos, uint8_t* remaining_utf8_continuation_bytes, program_options_t& options) {
        bool invalid_utf8_seq = false; 
        if (byte <= 127) {
                invalid_utf8_seq = (*remaining_utf8_continuation_bytes > 0);
                if (invalid_utf8_seq) {
                        char msg[256];
                        snprintf(msg, sizeof(msg), "expected %d UTF-8 continuation bytes, received byte 0x%0X", *remaining_utf8_continuation_bytes, (unsigned int)byte);
                        options.note_error(msg);
                } else {
                        if (options.print_byte_input()) {
                                printf("0x%02X - ", (int) byte);
                                printf("single-byte UTF-8 sequence: ");
                        }
                        describe_codepoint(byte, options);
                }
                /*
        } else if (byte == 0b11000000 || byte == 0b11000001) {
                options.note_error("UTF-8 invalid byte (would lead to overlong encoding of 0-127 range)");
                invalid_utf8_seq = true;
        } else if (byte >= 245) {
                options.note_error("UTF-8 invalid byte (if used as a starting byte would lead to value > 0x10FFFF)");
                invalid_utf8_seq = true;
                */
        } else if ((byte & /*0b11000000=*/0xc0) == /*0b10000000=*/0x80) {
                invalid_utf8_seq = (*remaining_utf8_continuation_bytes == 0);
                if (!invalid_utf8_seq) {
                        utf8_buffer[(*utf8_pos)++] = byte;
                        --*remaining_utf8_continuation_bytes;
                        if (*remaining_utf8_continuation_bytes == 0) {
                                uint8_t used_length = *utf8_pos;

                                utf8_buffer[used_length] = 0;
                                uint32_t code_point = utf8_sequence_to_codepoint(utf8_buffer, used_length);
                                if (code_point > 0x10FFFF) {
                                        char msg[256];
                                        snprintf(msg, sizeof(msg), " code point out of range: %u", code_point);
                                        options.note_error(msg);
                                } else if (((code_point <= 0x80) && used_length > 1)
                                                || (code_point < 0x800 && used_length > 2)
                                                || (code_point < 0x10000 && used_length > 3)) {
                                        char msg[256];
                                        snprintf(msg, sizeof(msg), " overlong encoding of %u using %d bytes", code_point, used_length);
                                        options.note_error(msg);
                                } else {
                                        if (options.print_byte_input()) {
                                                printf("0x%02X - ", (int) byte);
                                                printf("UTF-8 continuation byte 0b10xxxxxx: ");
                                        }
                                        describe_codepoint(code_point, options);
                                }
                                *remaining_utf8_continuation_bytes = 0;
                        } else if (options.print_byte_input()) {
                                printf("0x%02X - ", (int) byte);
                                printf("UTF-8 continuation byte 0b10xxxxxx\n");
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

                        char msg[256];
                        snprintf(msg, sizeof(msg), "invalid UTF-8 byte 0x%0X", byte);
                        options.note_error(msg);
                }
                if (expect_following != -1) {
                        if (*remaining_utf8_continuation_bytes == 0) {
                                if (options.print_byte_input()) {
                                        printf("UTF-8 leading byte      %s for a %d byte sequence\n",
                                                        (expect_following == 1) ? "0b110xxxxx" :
                                                        ((expect_following == 2) ? "0b1110xxxx" : "0b11110xxx"),
                                                        expect_following + 1);
                                }
                                *remaining_utf8_continuation_bytes = expect_following;
                                *utf8_pos = 1;
                                utf8_buffer[0] = byte;
                        } else {
                                invalid_utf8_seq = true;
                        }
                }
        }

        // FIXME: https://github.com/jackpal/Android-Terminal-Emulator/commit/7335c643f758ce4351ac00813027ce1505f8dbd4
        // - should continue with the current byte in the buffer
        if (invalid_utf8_seq) {
                *remaining_utf8_continuation_bytes = 0;
        }
}

void read_and_echo(program_options_t& options) {
        uint8_t remaining_utf8_continuation_bytes = 0;
        uint8_t utf8_pos = 0;
        uint8_t utf8_buffer[5];

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
                if (options.input_format == input_format_t::TEXTUAL_CODEPOINT) {
                        options.bytes_into_input += read_now;
                        start_of_buffer[start_of_buffer[read_now-1] == '\n' ? (read_now-1) : read_now] = 0;
                        if (read_now > 2 && start_of_buffer[0] == 'U' && read_buffer[1] == '+') {
                                // Handle "U+XXXX" as "0xXXXX"
                                start_of_buffer[0] = '0';
                                start_of_buffer[1] = 'x';
                        }

                        long codepoint = strtol((char*)start_of_buffer, NULL, 0);
                        if (codepoint == 0) {
                                char msg[sizeof(read_buffer)+1];
                                snprintf(msg, sizeof(msg), "cannot parse into code point: '%s'", start_of_buffer);
                                options.note_error(msg);
                        } else {
                                describe_codepoint(codepoint, options);

                        }
                } else {
                        for (int i = 0; i < read_now; i++) {
                                uint8_t c = start_of_buffer[i];
                                if (options.input_is_terminal && (c == 3 || c == 4)) {
                                        /* Let the user exit on ctrl+c or ctrl+d, or on end of file. */
                                        end_of_input = true;
                                        break;
                                } else {
                                        describe_byte(c, utf8_buffer, &utf8_pos, &remaining_utf8_continuation_bytes, options);
                                        options.bytes_into_input++;
                                        if (options.byte_skip_limit != 0 && ((options.byte_skip_limit + options.byte_skip_offset) <= options.bytes_into_input)) {
                                                end_of_input = true;
                                                break;
                                        }
                                }
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
                        "                                     * codepoint - decode input as textual U+XXXX descriptions\n"
                        "  -e, --encode-format FORMAT      Determine what output should be written:\n"
                        "                                     * codepoint - output code points in textual U+XXXX format\n"
                        "                                     * decoding (default) - debug output of the complete decoding process\n"
                        "                                     * silent - no output\n"
                        "                                     * utf8 - encode output as UTF-8\n"
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
                                } else if (strcmp(optarg, "utf32le") == 0) {
                                        options.output_format = output_format_t::UTF32LE;
                                } else if (strcmp(optarg, "utf32be") == 0) {
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
                int file_fd = open(argv[optind], O_RDONLY);
                if (file_fd < -1) {
                        perror("open()");
                        return EX_NOINPUT;
                }
                if (dup2(file_fd, STDIN_FILENO)) {
                        perror("dup2()");
                        return EX_IOERR;
                }
                close(file_fd);
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
                fprintf(stderr, "Summary: %" PRIu64 " code points decoded from %" PRIu64 " bytes with %" PRIu64 " errors\n", options.codepoints_into_input, options.bytes_into_input, options.error_count);
        } else if (is_output_tty && options.output_format != output_format_t::DESCRIPTION_CODEPOINT  && options.output_format != output_format_t::DESCRIPTION_DECODING && options.output_format != output_format_t::SILENT) {
                // Finish with newlines if writing raw output to terminal
                printf("\n");
        }

        int exit_status = options.error_count == 0 ? EX_OK : EX_DATAERR;
        options.cleanup_and_exit(exit_status);
}
