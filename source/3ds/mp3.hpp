#pragma once
#include <cstdint>
#include <cstddef>

class MP3 {
public:
    static bool init(const void* data, size_t size);
    static void cleanup();
    static size_t decode(void* buffer, size_t maxSize);
    static uint32_t getSampleRate();
    static uint8_t getChannels();
    static uint16_t getBitsPerSample();
    static size_t getBufferSize();
}; 