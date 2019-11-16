#include "utfdecode.hpp"

void print_usage_and_exit(char const *program_name, int exit_status) {
  fprintf(
      stderr,
      "usage: %s [OPTIONS] [file]\n"
      "  -b, --block-info             Show block and plane information. Only "
      "relevant if using the 'decoding' format\n"
      "  -d, --decode-format FORMAT   Determine how input should be "
      "decoded:\n"
      "                               * codepoint - decode input as "
      "textual U+XXXX descriptions\n"
      "                               * utf8 (default) - decode input as "
      "UTF-8\n"
      "                               * utf16le - decode input as "
      "UTF-16, little endian encoded\n"
      "                               * utf16be - decode input as "
      "UTF-16, big endian encoded\n"
      "                               * utf32le - decode input as "
      "UTF-32, little endian encoded\n"
      "                               * utf32be - decode input as "
      "UTF-32, big endian encoded\n"
      "  -e, --encode-format FORMAT   Determine how output should encoded. "
      "Accepts same as the above decoding formats and adds:\n"
      "                               * decoding (default) - debug "
      "output of the complete decoding process\n"
      "                               * silent - no output\n"
      "  -h, --help                   Show this help and exit\n"
      "  -l, --limit LIMIT            Only decode up to the specified amount "
      "of bytes\n"
      "  -m, --malformed <ACTION>     Determine what should happen on "
      "decoding error:\n"
      "                               * abort - abort the program "
      "directly with exit value 65\n"
      "                               * ignore - ignore invalid input\n"
      "                               * replace (default) - replace with "
      "the unicode replacement character � (U+FFFD)\n"
      "                               Note that errors are also logged to "
      "stderr unless -q is specified\n"
      "  -n, --normalization <FORM>   Specify normalization form to use: "
      "NFD, NFC, NFKD or NFKC\n"
      "  -o, --offset <OFFSET>        Skip the specified amount of bytes "
      "before starting decoding\n"
      "  -q, --quiet-errors           Do not log decoding errors to stderr\n"
      "  -s, --summary                Show a summary at end of input\n"
      "  -t, --timestamps             Show a timestamp after each input "
      "read\n"
      "  -w, --wcwidth                Show information about the wcwidth "
      "property for 'decoding'\n",
      program_name);
  exit(exit_status);
}

int main(int argc, char **argv) {
  setlocale(LC_ALL, NULL);
  program_options_t options;
  struct option getopt_options[] = {
      {"block-info", no_argument, nullptr, 'b'},
      {"encode-format", required_argument, nullptr, 'e'},
      {"decode-format", required_argument, nullptr, 'd'},
      {"help", no_argument, nullptr, 'h'},
      {"limit", required_argument, nullptr, 'l'},
      {"malformed", required_argument, nullptr, 'm'},
      {"normalization", required_argument, nullptr, 'n'},
      {"offset", required_argument, nullptr, 'o'},
      {"quiet-errors", no_argument, nullptr, 'q'},
      {"summary", no_argument, nullptr, 's'},
      {"timestamps", no_argument, nullptr, 't'},
      {"version", no_argument, nullptr, 'v'},
      {"wcwidth", no_argument, nullptr, 'w'},
      {0, 0, 0, 0}};

  while (true) {
    int option_index = 0;
    int c = getopt_long(argc, argv, "bd:e:hl:m:n:o:qstvw", getopt_options,
                        &option_index);
    if (c == -1)
      break;
    bool print_error_and_exit = false;
    int exit_status = EX_USAGE;
    switch (c) {
    case 'b':
      options.block_info = true;
      break;
    case 'd':
      if (strcmp(optarg, "utf8") == 0) {
        options.input_format = input_format_t::UTF8;
      } else if (strcmp(optarg, "utf16le") == 0 ||
                 strcmp(optarg, "utf16-le") == 0 ||
                 strcmp(optarg, "utf16") == 0 ||
                 strcmp(optarg, "utf-16") == 0) {
        options.input_format = input_format_t::UTF16LE;
      } else if (strcmp(optarg, "utf16be") == 0 ||
                 strcmp(optarg, "utf16-be") == 0) {
        options.input_format = input_format_t::UTF16BE;
      } else if (strcmp(optarg, "utf32le") == 0 ||
                 strcmp(optarg, "utf32-le") == 0 ||
                 strcmp(optarg, "utf32") == 0 ||
                 strcmp(optarg, "utf-32") == 0) {
        options.input_format = input_format_t::UTF32LE;
      } else if (strcmp(optarg, "utf32be") == 0 ||
                 strcmp(optarg, "utf32-be") == 0) {
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
      } else if (strcmp(optarg, "utf16le") == 0 ||
                 strcmp(optarg, "utf16-le") == 0 ||
                 strcmp(optarg, "utf16") == 0 ||
                 strcmp(optarg, "utf-16") == 0) {
        options.output_format = output_format_t::UTF16LE;
      } else if (strcmp(optarg, "utf16be") == 0 ||
                 strcmp(optarg, "utf16-be") == 0) {
        options.output_format = output_format_t::UTF16BE;
      } else if (strcmp(optarg, "utf32le") == 0 ||
                 strcmp(optarg, "utf32-le") == 0 ||
                 strcmp(optarg, "utf32") == 0 ||
                 strcmp(optarg, "utf-32") == 0) {
        options.output_format = output_format_t::UTF32LE;
      } else if (strcmp(optarg, "utf32be") == 0 ||
                 strcmp(optarg, "utf32-be") == 0) {
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
        options.error_handling = error_handling_t::REPLACE;
        ;
      } else if (strcmp(optarg, "abort") == 0) {
        options.error_handling = error_handling_t::ABORT;
      } else {
        fprintf(stderr, "'%s' is not a valid error handling\n", optarg);
        print_error_and_exit = true;
      }
      break;
    case 'n':
      if (strcmp(optarg, "NFD") == 0 || strcmp(optarg, "nfd") == 0) {
        options.normalization_form = normalization_form_t::NFD;
      } else if (strcmp(optarg, "NFC") == 0 || strcmp(optarg, "nfc") == 0) {
        options.normalization_form = normalization_form_t::NFC;
      } else if (strcmp(optarg, "NFKD") == 0 || strcmp(optarg, "nfkd") == 0) {
        options.normalization_form = normalization_form_t::NFKD;
      } else if (strcmp(optarg, "NFKC") == 0 || strcmp(optarg, "nfkc") == 0) {
        options.normalization_form = normalization_form_t::NFKC;
      } else {
        fprintf(stderr, "'%s' is not a valid normalization form\n", optarg);
        print_error_and_exit = true;
      }
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
    case 'v':
      printf("%s\n", PACKAGE_VERSION);
      return 0;
      break;
    case 'w':
      options.wcwidth = true;
      break;
    default:
      print_error_and_exit = true;
      break;
    }
    if (print_error_and_exit)
      print_usage_and_exit(argv[0], exit_status);
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

  options.output_is_terminal = isatty(STDOUT_FILENO);

  // If inputting textual code points, do not special case terminal input:
  options.input_is_terminal =
      (options.input_format != input_format_t::TEXTUAL_CODEPOINT) &&
      isatty(STDIN_FILENO);

  if (options.input_is_terminal) {
    tcgetattr(0, &options.vt_orig);
    struct termios vt_new = options.vt_orig;
    // Minimum number of characters for noncanonical read (MIN).
    vt_new.c_cc[VMIN] = 1;
    // Timeout in deciseconds for noncanonical read (TIME).
    vt_new.c_cc[VTIME] = 0;
    // echo, canonical mode, extended input processing and signal chars off:
    vt_new.c_lflag &= IEXTEN | ISIG;

    tcsetattr(STDIN_FILENO, TCSANOW, &vt_new);
  }

  unicode_code_points_initialize();

  options.read_and_echo();

  options.flush_normalization_non_starters(options.normalization_non_starters);

  if (options.print_summary) {
    char const *color_prefix = options.output_is_terminal ? "\x1B[35m" : "";
    char const *color_suffix = options.output_is_terminal ? "\x1B[m" : "";
    uint64_t bytes_encountered =
        options.bytes_into_input - options.byte_skip_offset;
    fprintf(stderr,
            "%s%" PRIu64 " code points from %" PRIu64 " bytes with %" PRIu64
            " errors%s\n",
            color_prefix, options.codepoints_into_input, bytes_encountered,
            options.error_count, color_suffix);
  } else if (options.output_is_terminal &&
             options.output_format != output_format_t::DESCRIPTION_CODEPOINT &&
             options.output_format != output_format_t::DESCRIPTION_DECODING &&
             options.output_format != output_format_t::SILENT) {
    printf("\n");
  }

  int exit_status = options.error_count == 0 ? EX_OK : EX_DATAERR;
  options.cleanup_and_exit(exit_status);
}
