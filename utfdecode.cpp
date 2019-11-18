#include "utfdecode.hpp"

void die_with_internal_error [[noreturn]] (char const *fmt, ...) {
  fprintf(stderr, "utfdecode: internal error - ");
  va_list argp;
  va_start(argp, fmt);
  vfprintf(stderr, fmt, argp);
  va_end(argp);
  fprintf(stderr, "\n");
  exit(EX_SOFTWARE);
}

void program_options_t::encode_codepoint(uint32_t codepoint,
                                         bool output_non_starters) {
  this->codepoints_into_input++;

  if (this->is_silent_output()) {
    return;
  }

  auto code_point_info = lookup_code_point(codepoint);

  if (this->normalization_form != normalization_form_t::NONE) {
    bool compatible = this->normalization_form == normalization_form_t::NFKC ||
                      this->normalization_form == normalization_form_t::NFKD;

    uint8_t len;
    uint32_t const *decomposed = unicode_decompose(codepoint, compatible, &len);
    if (len == 0) {
      if (!output_non_starters) {
        bool is_starter = code_point_info->canonical_combining_class == 0;
        if (is_starter) {
          std::vector<uint32_t> non_starters_copy =
              this->normalization_non_starters;
          this->normalization_non_starters.clear();

          flush_normalization_non_starters(non_starters_copy);
        } else {
          this->normalization_non_starters.push_back(codepoint);
          return;
        }
      }
    } else {
      // TODO: Recursive decomposition
      for (uint8_t i = 0; i < len; i++) {
        this->encode_codepoint(decomposed[i], true);
      }
      return;
    }
  }

  if (this->output_format == output_format_t::DESCRIPTION_DECODING ||
      this->output_format == output_format_t::DESCRIPTION_CODEPOINT) {
    uint8_t utf8_buffer[5];
    int utf8_byte_count = codepoint_to_utf8(codepoint, utf8_buffer);
    utf8_buffer[utf8_byte_count] = 0;
    int wcwidth_value = wcwidth_musl(codepoint);
    if (wcwidth_value != -1) {
      char const *extra_whitespace =
          general_category_is_combining(code_point_info->category) ? " " : "";
      printf("%s%s = ", extra_whitespace, utf8_buffer);
    }

    auto name = lookup_code_point_name(codepoint);
    printf("U+%04X %s", codepoint, name.c_str());

    if (this->block_info) {
      char const *plane_name = "???";
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

      char const *block_name = get_block_name(codepoint);
      printf(". Block %s in plane %s", block_name, plane_name);

      char const *category_description =
          general_category_description(code_point_info->category);
      printf(". Category: %s", category_description);
    }
    if (this->wcwidth) {
      printf(". wcwidth=%d", wcwidth_value);
    }
    printf("\n");
  } else if (this->output_format == output_format_t::UTF8) {
    uint8_t utf8_buffer[5];
    int utf8_byte_count = codepoint_to_utf8(codepoint, utf8_buffer);
    utf8_buffer[utf8_byte_count] = 0;
    printf("%s", utf8_buffer);
    fflush(stdout);
  } else if (output_format == output_format_t::UTF16BE ||
             output_format == output_format_t::UTF16LE) {
    uint8_t output[5];
    bool little_endian = output_format == output_format_t::UTF16LE;
    int output_length = encode_utf16(codepoint, output, little_endian);
    write(STDOUT_FILENO, output, output_length);
    fflush(stdout);
  } else if (output_format == output_format_t::UTF32BE ||
             output_format == output_format_t::UTF32LE) {
    uint8_t buffer[5];
    bool little_endian = output_format == output_format_t::UTF32LE;
    for (int i = 0; i < 4; i++) {
      int shift = 8 * (little_endian ? i : (3 - i));
      buffer[i] = (codepoint >> shift) & 0xFF;
    }
    write(STDOUT_FILENO, buffer, 4);
    fflush(stdout);
  }
}

void program_options_t::flush_normalization_non_starters(
    std::vector<uint32_t> &non_starters) {
  std::sort(non_starters.begin(), non_starters.end(),
            [](uint32_t a, uint32_t b) {
              auto ac = lookup_code_point(a)->canonical_combining_class;
              auto bc = lookup_code_point(b)->canonical_combining_class;
              return ac < bc;
            });

  for (auto cp : non_starters) {
    encode_codepoint(cp, true);
  }
}

void program_options_t::note_error(int byte, char const *error_msg, ...) {
  error_count++;
  if (error_reporting == error_reporting_t::REPORT_STDERR) {
    fflush(stdout);
    char const *color_prefix = output_is_terminal ? "\x1B[31m" : "";
    char const *color_suffix = output_is_terminal ? "\x1B[m" : "";
    fprintf(stderr, "%s", color_prefix);
    fprintf(stderr, "malformed %" PRIu64 " %s and %" PRIu64 " %s in - ",
            bytes_into_input, (bytes_into_input == 1 ? "byte" : "bytes"),
            codepoints_into_input,
            codepoints_into_input == 1 ? "character" : "characters");
    va_list argp;
    va_start(argp, error_msg);
    vfprintf(stderr, error_msg, argp);
    va_end(argp);

    fprintf(stderr, "%s\n", color_suffix);
    fflush(stderr);
  }

  switch (error_handling) {
  case error_handling_t::IGNORE:
    break;
  case error_handling_t::REPLACE:
    if (!print_byte_input()) {
      encode_codepoint(0xFFFD);
    }
    break;
  case error_handling_t::ABORT:
    exit(EX_DATAERR);
    break;
  }
}

void program_options_t::cleanup_and_exit(int exit_status) {
  if (input_is_terminal)
    tcsetattr(0, TCSANOW, &vt_orig);
  exit(exit_status);
}

void program_options_t::print_byte_result(int byte, char const *msg, ...) {
  if (print_byte_input()) {
    va_list argp;
    va_start(argp, msg);
    vfprintf(stdout, msg, argp);
    va_end(argp);
  }
}

void program_options_t::process_utf16_byte(uint8_t byte, uint8_t *state_buffer,
                                           uint8_t &state_pos) {
  state_buffer[state_pos++] = byte;
  if (state_pos == 2) {
    uint16_t codeuint =
        (input_format == input_format_t::UTF16LE)
            ? state_buffer[0] + (uint16_t(state_buffer[1]) << 8)
            : state_buffer[1] + (uint16_t(state_buffer[0]) << 8);
    if (codeuint >= 0xD800 && codeuint <= 0xDBFF) {
      print_byte_result(byte, "leading surrogate %d\n", codeuint);
    } else if (codeuint >= 0xDC00 && codeuint <= 0xDFFF) {
      note_error(byte, "trailing surrogate %d without leading surrogate before",
                 codeuint);
      state_pos = 0;
    } else {
      encode_codepoint(codeuint);
      state_pos = 0;
    }
  } else if (state_pos == 4) {
    uint16_t leading_surrogate =
        (input_format == input_format_t::UTF16LE)
            ? state_buffer[0] + (uint16_t(state_buffer[1]) << 8)
            : state_buffer[1] + (uint16_t(state_buffer[0]) << 8);
    uint16_t trailing_surrogate =
        (input_format == input_format_t::UTF16LE)
            ? state_buffer[2] + (uint16_t(state_buffer[3]) << 8)
            : state_buffer[3] + (uint16_t(state_buffer[2]) << 8);
    uint32_t codepoint = 0x010000 +
                         (uint32_t(leading_surrogate - 0xD800) << 10) +
                         (trailing_surrogate - 0xDC00);
    print_byte_result(byte, "trailing surrogate %d", trailing_surrogate);
    encode_codepoint(codepoint);
    state_pos = 0;
  }
}

void program_options_t::process_utf32_byte(uint8_t byte, uint8_t *state_buffer,
                                           uint8_t &state_pos) {
  state_buffer[state_pos++] = byte;
  if (state_pos == 4) {
    uint32_t codepoint = 0;
    for (int i = 0; i < 4; i++) {
      int shift = 8 * ((input_format == input_format_t::UTF32LE) ? i : (3 - i));
      codepoint += (state_buffer[i] << shift) & (0xFF << shift);
    }
    print_byte_result(byte, "byte 4 of a UTF-32 code unit: ");
    encode_codepoint(codepoint);
    state_pos = 0;
  } else {
    print_byte_result(byte, "byte %d of a UTF-32 code unit\n", state_pos);
  }
}

void program_options_t::process_textual_codepoint_byte(
    uint8_t byte, uint8_t *state_buffer, uint8_t &state_buffer_pos) {
  if (byte == '\n' || byte == ' ' || byte == '\r' || byte == '\t') {
    if (state_buffer_pos > 0) {
      if (state_buffer_pos > 2 && state_buffer[0] == 'U' &&
          state_buffer[1] == '+') {
        // Handle "U+XXXX" as "0xXXXX"
        state_buffer[0] = '0';
        state_buffer[1] = 'x';
      }
      state_buffer[state_buffer_pos] = 0;
      long codepoint = strtol((char *)state_buffer, NULL, 0);
      if (codepoint == 0) {
        note_error(byte, "cannot parse into code point: '%s'", state_buffer);
      } else {
        encode_codepoint(codepoint);
      }
      state_buffer_pos = 0;
    }
  } else if (state_buffer_pos >= 16) {
    state_buffer[state_buffer_pos] = 0;
    note_error(byte, "too large string '%s' - discarding", state_buffer);
  } else {
    state_buffer[state_buffer_pos++] = byte;
  }
}

void program_options_t::read_and_echo() {
  uint8_t remaining_bytes = 0;
  uint8_t state_buffer_position = 0;
  uint8_t state_buffer[16];

  int64_t initial_timestamp = -1;
  if (timestamps) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    initial_timestamp = tv.tv_sec * 1000 + tv.tv_usec / 1000;
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

    unsigned char *start_of_buffer = read_buffer;
    if (read_now + bytes_into_input < byte_skip_offset) {
      bytes_into_input += read_now;
      continue;
    } else if (bytes_into_input < byte_skip_offset) {
      int skipping_now = byte_skip_offset - bytes_into_input;
      bytes_into_input += skipping_now;
      read_now -= skipping_now;
      start_of_buffer = &read_buffer[skipping_now];
    }

    if (timestamps) {
      struct timeval tv;
      gettimeofday(&tv, NULL);
      long long int elapsed =
          (tv.tv_sec * 1000 + tv.tv_usec / 1000) - initial_timestamp;
      printf("%lld ms\n", elapsed);
    }

    bool end_of_input = false;
    for (int i = 0; i < read_now; i++) {
      uint8_t c = start_of_buffer[i];
      if (input_is_terminal && (c == 3 || c == 4)) {
        /* Let the user exit on ctrl+c or ctrl+d, or on end of file. */
        end_of_input = true;
        break;
      }
      switch (input_format) {
      case input_format_t::UTF8:
        process_utf8_byte(c, state_buffer, state_buffer_position,
                          remaining_bytes);
        break;
      case input_format_t::UTF16BE:
      case input_format_t::UTF16LE:
        process_utf16_byte(c, state_buffer, state_buffer_position);
        break;
      case input_format_t::UTF32BE:
      case input_format_t::UTF32LE:
        process_utf32_byte(c, state_buffer, state_buffer_position);
        break;
      case input_format_t::TEXTUAL_CODEPOINT:
        process_textual_codepoint_byte(c, state_buffer, state_buffer_position);
        break;
      }
      bytes_into_input++;
      if (byte_skip_limit != 0 &&
          ((byte_skip_limit + byte_skip_offset) <= bytes_into_input)) {
        end_of_input = true;
        break;
      }
    }
    fflush(stdout);
    if (end_of_input)
      return;
  }
}
