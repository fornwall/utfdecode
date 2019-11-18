#include "utfdecode.hpp"

int codepoint_to_utf8(uint32_t codePoint, uint8_t *utf8InputBuffer) {
  int bufferPosition = 0;
  if (codePoint <= /* 7 bits */ 0b1111111) {
    utf8InputBuffer[bufferPosition++] = (uint8_t)codePoint;
  } else if (codePoint <= /* 11 bits */ 0b11111111111) {
    /* 110xxxxx leading byte with leading 5 bits */
    utf8InputBuffer[bufferPosition++] =
        (uint8_t)(0b11000000 | (codePoint >> 6));
    /* 10xxxxxx continuation byte with following 6 bits */
    utf8InputBuffer[bufferPosition++] =
        (uint8_t)(0b10000000 | (codePoint & 0b111111));
  } else if (codePoint <= /* 16 bits */ 0b1111111111111111) {
    /* 1110xxxx leading byte with leading 4 bits */
    utf8InputBuffer[bufferPosition++] =
        (uint8_t)(0b11100000 | (codePoint >> 12));
    /* 10xxxxxx continuation byte with following 6 bits */
    utf8InputBuffer[bufferPosition++] =
        (uint8_t)(0b10000000 | ((codePoint >> 6) & 0b111111));
    /* 10xxxxxx continuation byte with following 6 bits */
    utf8InputBuffer[bufferPosition++] =
        (uint8_t)(0b10000000 | (codePoint & 0b111111));
  } else if (codePoint <= /* 21 bits */ 0b111111111111111111111) {
    /* 11110xxx leading byte with leading 3 bits */
    utf8InputBuffer[bufferPosition++] =
        (uint8_t)(0b11110000 | (codePoint >> 18));
    /* 10xxxxxx continuation byte with following 6 bits */
    utf8InputBuffer[bufferPosition++] =
        (uint8_t)(0b10000000 | ((codePoint >> 12) & 0b111111));
    /* 10xxxxxx continuation byte with following 6 bits */
    utf8InputBuffer[bufferPosition++] =
        (uint8_t)(0b10000000 | ((codePoint >> 6) & 0b111111));
    /* 10xxxxxx continuation byte with following 6 bits */
    utf8InputBuffer[bufferPosition++] =
        (uint8_t)(0b10000000 | (codePoint & 0b111111));
  } else {
    die_with_internal_error("codepoint_to_utf8(): invalid code point %d",
                            codePoint);
  }
  return bufferPosition;
}

/* Assumes that the buffer is already validated. Does not handle overlong
 * encodings. */
uint32_t utf8_sequence_to_codepoint(uint8_t *buffer, uint8_t length) {
  uint32_t code_point;
  uint32_t first_byte_mask;
  switch (length) {
  case 2:
    first_byte_mask = 0x1F;
    break;
  case 3:
    first_byte_mask = 0x0F;
    break;
  case 4:
    first_byte_mask = 0x07;
    break;
  default:
    die_with_internal_error("utf8_sequence_to_codepoint(): length=%d\n",
                            (int)length);
  }
  code_point = (buffer[0] & first_byte_mask);
  for (int i = 1; i < length; i++)
    code_point = ((code_point << 6) | (uint32_t)(buffer[i] & 0x3F));
  return code_point;
}

void program_options_t::process_utf8_byte(uint8_t byte, uint8_t *utf8_buffer, uint8_t &utf8_pos,
                       uint8_t &remaining_utf8_continuation_bytes) {
  bool invalid_utf8_seq = false;
  if (byte <= 127) {
    invalid_utf8_seq = (remaining_utf8_continuation_bytes > 0);
    if (invalid_utf8_seq) {
      note_error(byte, "expected continuation byte");
    } else {
      encode_codepoint(byte);
    }
  } else if ((byte & /*0b11000000=*/0xc0) == /*0b10000000=*/0x80) {
    invalid_utf8_seq = (remaining_utf8_continuation_bytes == 0);
    if (!invalid_utf8_seq) {
      utf8_buffer[utf8_pos++] = byte;
      --remaining_utf8_continuation_bytes;
      if (remaining_utf8_continuation_bytes == 0) {
        uint8_t used_length = utf8_pos;

        utf8_buffer[used_length] = 0;
        uint32_t code_point =
            utf8_sequence_to_codepoint(utf8_buffer, used_length);
        if (code_point > 0x10FFFF) {
          note_error(byte, "code point out of range: %u", code_point);
        } else if (code_point >= 0xD800 && code_point <= 0xDFFF) {
          note_error(byte, "surrogate %u in UTF-8", code_point);
        } else if (((code_point <= 0x80) && used_length > 1) ||
                   (code_point < 0x800 && used_length > 2) ||
                   (code_point < 0x10000 && used_length > 3)) {
          note_error(byte, "overlong encoding of %u using %d bytes",
                             code_point, used_length);
        } else {
          encode_codepoint(code_point);
        }
        remaining_utf8_continuation_bytes = 0;
      }
    }
  } else {
    int expect_following;
    if ((byte & /*0b11100000=*/0xe0) == /*0b11000000=*/0xc0) {
      expect_following = 1;
    } else if ((byte & /*0b11110000=*/0xf0) == /*0b11100000=*/0xe0) {
      expect_following = 2;
    } else if ((byte & /*0b11111000=*/0xf8) == /*0b11110000=*/0xf0) {
      expect_following = 3;
    } else {
      expect_following = -1;
      invalid_utf8_seq = true;
      note_error(byte, "invalid byte");
    }
    if (expect_following != -1) {
      if (remaining_utf8_continuation_bytes == 0) {
        remaining_utf8_continuation_bytes = expect_following;
        utf8_pos = 1;
        utf8_buffer[0] = byte;
      } else {
        invalid_utf8_seq = true;
      }
    }
  }

  // FIXME:
  // https://github.com/jackpal/Android-Terminal-Emulator/commit/7335c643f758ce4351ac00813027ce1505f8dbd4
  // - should continue with the current byte in the buffer
  if (invalid_utf8_seq) {
    remaining_utf8_continuation_bytes = 0;
  }
}

