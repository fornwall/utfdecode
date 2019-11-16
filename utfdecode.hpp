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

static void die_with_internal_error [[noreturn]] (char const *fmt, ...) {
  fprintf(stderr, "utfdecode: internal error - ");
  va_list argp;
  va_start(argp, fmt);
  vfprintf(stderr, fmt, argp);
  va_end(argp);
  fprintf(stderr, "\n");
  exit(EX_SOFTWARE);
}

void unicode_code_points_initialize();
code_point const* lookup_code_point(uint32_t);
uint32_t const* unicode_decompose(uint32_t codePoint, bool compatible, uint8_t* len);
int codepoint_to_utf8(uint32_t codePoint, uint8_t *utf8InputBuffer);
int encode_utf16(uint32_t codePoint, uint8_t *buffer, bool little_endian);
char const *general_category_description(general_category_value_t category);
char const* get_block_name(uint32_t codepoint);

#endif
