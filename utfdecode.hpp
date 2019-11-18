#ifndef UTFDECODE_HPP_INCLUDED
#define UTFDECODE_HPP_INCLUDED

#define __STDC_FORMAT_MACROS
#define _XOPEN_SOURCE

#include <fcntl.h>
#include <getopt.h>
#include <inttypes.h>
#include <locale.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sysexits.h>
#include <termios.h>
#include <unistd.h>
#include <wchar.h>

#include <algorithm>
#include <string>
#include <vector>

#include "config.h"

#include "musl/wcwidth_musl.h"

enum class normalization_form_t {
  NONE, // Do not use a normalization form.
  NFD,  // Canonical Decomposition.
  NFC,  // Canonical Decomposition, followed by Canonical Composition.
  NFKD, // Compatibility Decomposition
  NFKC  // Compatibility Decomposition, followed by Canonical Composition
};

enum class general_category_value_t {
  Uppercase_Letter,
  Lowercase_Letter,
  Titlecase_Letter,
  Cased_Letter,
  Modifier_Letter,
  Other_Letter,
  Letter,
  Nonspacing_Mark,
  Spacing_Mark,
  Enclosing_Mark,
  Mark,
  Decimal_Number,
  Letter_Number,
  Other_Number,
  Number,
  Connector_Punctuation,
  Dash_Punctuation,
  Open_Punctuation,
  Close_Punctuation,
  Initial_Punctuation,
  Final_Punctuation,
  Other_Punctuation,
  Punctuation,
  Math_Symbol,
  Currency_Symbol,
  Modifier_Symbol,
  Other_Symbol,
  Symbol,
  Space_Separator,
  Line_Separator,
  Paragraph_Separator,
  Separator,
  Control,
  Format,
  Surrogate,
  Private_Use,
  Unassigned,
  Other
};

struct code_point {
  uint32_t numeric_value;
  char const* name;
  general_category_value_t category;
  uint8_t canonical_combining_class;
  bool bidi_mirrored;
  uint32_t simple_uppercase_mapping;
  uint32_t simple_lowercase_mapping;
  uint32_t simple_titlecase_mapping;
};

enum class input_format_t {
  UTF8,
  UTF16BE,
  UTF16LE,
  UTF32LE,
  UTF32BE,
  TEXTUAL_CODEPOINT
};

enum class output_format_t {
  UTF8,
  UTF16BE,
  UTF16LE,
  UTF32LE,
  UTF32BE,
  DESCRIPTION_CODEPOINT,
  DESCRIPTION_DECODING,
  SILENT
};

enum class error_handling_t { ABORT, REPLACE, IGNORE };

enum class error_reporting_t { REPORT_STDERR, SILENT };

void die_with_internal_error [[noreturn]] (char const *fmt, ...);

void unicode_code_points_initialize();

code_point const *lookup_code_point(uint32_t);

std::string lookup_code_point_name(uint32_t code_point);

uint32_t const *unicode_decompose(uint32_t codePoint, bool compatible,
                                  uint8_t *len);

int codepoint_to_utf8(uint32_t codePoint, uint8_t *utf8InputBuffer);

int encode_utf16(uint32_t codePoint, uint8_t *buffer, bool little_endian);

char const *general_category_description(general_category_value_t category);

bool general_category_is_combining(general_category_value_t category);

char const *get_block_name(uint32_t codepoint);

struct program_options_t {
  input_format_t input_format{input_format_t::UTF8};
  output_format_t output_format{output_format_t::DESCRIPTION_DECODING};
  error_handling_t error_handling{error_handling_t::REPLACE};
  error_reporting_t error_reporting{error_reporting_t::REPORT_STDERR};
  normalization_form_t normalization_form{normalization_form_t::NONE};
  bool timestamps{false};
  bool print_summary{false};
  bool output_is_terminal{false};
  bool input_is_terminal{false};
  bool block_info{false};
  bool wcwidth{false};

  struct termios vt_orig;

  uint64_t byte_skip_offset{0};
  uint64_t byte_skip_limit{0};
  uint64_t bytes_into_input{0};
  uint64_t codepoints_into_input{0};
  uint64_t error_count{0};

  std::vector<uint32_t> normalization_non_starters;

  bool print_byte_input() const {
    return output_format == output_format_t::DESCRIPTION_DECODING &&
           input_format != input_format_t::TEXTUAL_CODEPOINT;
  };

  bool is_silent_output() const {
    return output_format == output_format_t::SILENT;
  };

  bool is_replace_errors() {
    return error_handling == error_handling_t::REPLACE;
  }

  void encode_codepoint(uint32_t codepoint, bool output_non_starters = false);

  void flush_normalization_non_starters(std::vector<uint32_t> &non_starters);

  void note_error(int byte, char const *error_msg, ...);

  void cleanup_and_exit(int exit_status);

  void print_byte_result(int byte, char const *msg, ...);

  void process_utf8_byte(uint8_t byte, uint8_t *utf8_buffer, uint8_t &utf8_pos,
                       uint8_t &remaining_utf8_continuation_bytes);

  void process_utf16_byte(uint8_t byte, uint8_t *state_buffer, uint8_t &state_pos);

  void process_utf32_byte(uint8_t byte, uint8_t *state_buffer, uint8_t &state_pos);

  void process_textual_codepoint_byte(uint8_t byte, uint8_t *state_buffer,
                                    uint8_t &state_buffer_pos);

  void read_and_echo();
};

#endif
