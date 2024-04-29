#include "ring_buffer.hpp"
#include <cstdlib>
#include <string.h>

constexpr uint8_t RING_BUFFER_VALUE_RANGE = 255;

MostCommonElementRingBuffer::MostCommonElementRingBuffer()
	: m_elements(nullptr), m_counter(nullptr), m_size(0), m_write(0), m_occupiedSize(0) {}


MostCommonElementRingBuffer::MostCommonElementRingBuffer(const uint32_t size)
	: m_elements(nullptr), m_counter(nullptr), m_size(size), m_write(0), m_occupiedSize(0)
{
	m_elements = static_cast<uint8_t*>(malloc(size * sizeof(uint8_t)));
	if (m_elements != nullptr) {
		memset(m_elements, 0xFF, size * sizeof(uint8_t));

		m_counter = static_cast<uint32_t*>(malloc(RING_BUFFER_VALUE_RANGE * sizeof(uint32_t)));
		if (m_counter != nullptr) {
			memset(m_counter, 0, RING_BUFFER_VALUE_RANGE * sizeof(uint32_t));
		}
	}
}
void MostCommonElementRingBuffer::Init(const uint32_t size)
{
	m_elements		= nullptr;
	m_counter		= nullptr;
	m_size			= size;
	m_write			= 0;
	m_occupiedSize	= 0;
	
	m_elements = static_cast<uint8_t*>(malloc(size * sizeof(uint8_t)));

	if (m_elements != nullptr) {
		memset(m_elements, 0xFF, size * sizeof(uint8_t));

		m_counter = static_cast<uint32_t*>(malloc(RING_BUFFER_VALUE_RANGE * sizeof(uint32_t)));
		if (m_counter != nullptr) {
			memset(m_counter, 0, RING_BUFFER_VALUE_RANGE * sizeof(uint32_t));
		}
	}
}

MostCommonElementRingBuffer::~MostCommonElementRingBuffer() {
	if (m_counter != nullptr) {
		free(m_counter);
		m_counter = nullptr;
	}
	if (m_elements != nullptr) {
		free(m_elements);
		m_elements = nullptr;
		m_write = -1;
	}
}

void MostCommonElementRingBuffer::Reset() {
	if (m_elements != nullptr) {
		memset(m_elements, 0, m_size * sizeof(uint8_t));
		if (m_counter != nullptr) {
			memset(m_counter, 0, RING_BUFFER_VALUE_RANGE * sizeof(uint32_t));
		}
	}
}

void MostCommonElementRingBuffer::Push(const uint8_t value) {
	if (m_elements != nullptr && m_counter != nullptr) {

		// Increase the occupied size until we fill the buffer
		if (m_occupiedSize < m_size) {
			m_occupiedSize++;
		} else {
			// buffer is full, handle overwriting
			uint8_t oldValue = m_elements[m_write];
			// Decrease last value
			m_counter[oldValue]--;
		}

		// Write element, increment counter, update the write pointer
		m_elements[m_write] = value;
		m_counter[value]++;
		m_write = (m_write + 1) % m_size;
	}
}

const uint8_t MostCommonElementRingBuffer::MostCommonElement() const {
	if (m_elements != nullptr && m_counter != nullptr) {

		uint32_t max_count = 0;
		uint8_t max_value = 0;

		// Go through the counter list and find the largest value
		for (int i = 0; i < RING_BUFFER_VALUE_RANGE; i++) {
			if (m_counter[i] > max_count) {
				max_count = m_counter[i];
				max_value = i;
			}
		}

		return max_value;
	} else {
		return 0xFF; // INVALID VALUE
	}
}

// @TODO: You can probably implement this in such a way that we only search the loop whenever we update a value
//        simply by checking if the new value is larger or smaller than the last value. We would also have to keep track
//        of how many instances of the most common number are in the list internally and search if it's smaller