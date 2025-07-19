#pragma once
#include <vector>
#include <string>
#include <cstdint>

struct AudioTrack {
    void* buffer;
    void* buffer2;
    const char* audioData;
    size_t size;
    size_t bufferSize;
    bool isPlaying;
    int channel;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint16_t bitsPerSample;
    size_t currentPos;
    size_t samplesPerChannel;
    char waveBuf[2 * 64];  // Space for two ndspWaveBuf structures (approximate size)
};

class Audio {
public:
    static bool init();
    static void cleanup();
    static int loadWAV(const void* data, size_t size);
    static int loadMP3(const void* data, size_t size);
    static void playTrack(int trackId, bool loop = false);
    static void stopTrack(int trackId);
    static void stopAllTracks();
    static bool isTrackPlaying(int trackId);
    static void update();
}; 