#include "cobs.hpp"
#include <cinttypes>

void encode(const uint8_t* ptr, uint32_t length, uint8_t* dst) {
	#define FinishBlock(X) (*code_ptr = (X), code_ptr = dst++, code = 0x01)

	const uint8_t* end = ptr + length;
	uint8_t* code_ptr = dst++;
	uint8_t code = 0x01;
	while (ptr < end)
	{
		if (*ptr == 0) {
			FinishBlock(code);
		} else {
			*dst++ = *ptr;
			code++;
			if (code == 0xFF) FinishBlock(code);
		}
		ptr++;
	}
	FinishBlock(code);

	#undef FinishBlock
}

size_t cobs::decode(uint8_t* buffer, const size_t size) {

	// COBS can only encode up to 254 bytes
	if (size < 1 || size > 255) {
		return 0;
	}

	uint8_t* bufferSrc = buffer;

	const uint8_t* end = bufferSrc + size;
	while (bufferSrc < end) {
		int i, code = *bufferSrc++;
		for (i = 1; i < code; i++) {
			*buffer++ = *bufferSrc++;
		}
		if (code < 0xFF) {
			*buffer++ = 0;
		}
	}

	return size - 1;
}