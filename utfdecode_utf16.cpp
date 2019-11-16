#include "utfdecode.hpp"

int encode_utf16(uint32_t codePoint, uint8_t *buffer, bool little_endian) {
	if (codePoint <= 0xFFFF) {
		buffer[0] = little_endian ? (codePoint & 0xFF) : (codePoint >> 8);
		buffer[1] = little_endian ? (codePoint >> 8) : (codePoint & 0xFF);
		buffer[2] = 0;
		return 2;
	} else {
		// Code points from the other planes (called Supplementary Planes) are
		// encoded in UTF-16 by pairs of 16-bit code units called surrogate
		// pairs, by the following scheme: (1) 0x010000 is subtracted from the
		// code point, leaving a 20 bit number in the range 0..0x0FFFFF. (2) The
		// top ten bits (a number in the range 0..0x03FF) are added to 0xD800 to
		// give the first code unit or lead surrogate, which will be in the
		// range 0xD800..0xDBFF. (3) The low ten bits (also in the range
		// 0..0x03FF) are added to 0xDC00 to give the second code unit or trail
		// surrogate, which will be in the range 0xDC00..0xDFFF.
		codePoint -= 0x010000;
		uint16_t first = (codePoint >> 10) + 0xD800;
		uint16_t second = (0b1111111111 & codePoint) + 0xDC00;
		return encode_utf16(first, buffer, little_endian) + encode_utf16(second, buffer + 2, little_endian);
	}
}
