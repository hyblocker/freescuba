#pragma once

#include <cinttypes>

/// <summary>
/// A most common element ring buffer that can store up to N elements.
/// </summary>
class MostCommonElementRingBuffer {

public:
	MostCommonElementRingBuffer();
	MostCommonElementRingBuffer(const uint32_t size);
	~MostCommonElementRingBuffer();

	void Init(const uint32_t size);
	void Reset();
	void Push(const uint8_t value);
	const uint8_t MostCommonElement() const;
	inline bool IsValid() const { return m_elements != nullptr; }

private:
	uint8_t* m_elements;
	uint32_t* m_counter;

	uint32_t m_size;
	uint32_t m_occupiedSize;
	uint32_t m_write;
};