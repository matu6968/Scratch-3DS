#include "audio.hpp"
#include <3ds.h>
#include <3ds/ndsp/ndsp.h>
#include <3ds/ndsp/channel.h>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <thread>
#include <chrono>

// Audio constants
#define AUDIO_CHANNEL 0x08
#define BUFFER_SIZE (16 * 1024)

// Audio buffers
static bool audioInitialized = false;
static std::vector<AudioTrack> audioTracks;

// Simple WAV header structure
struct WAVHeader {
    char riff[4];
    uint32_t fileSize;
    char wave[4];
    char fmt[4];
    uint32_t fmtSize;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    char data[4];
    uint32_t dataSize;
};

bool Audio::init() {
    if (audioInitialized) return true;
    
    std::cout << "Initializing 3DS audio system..." << std::endl;
    
    // Proper NDSP initialization
    Result rc = ndspInit();
    if (R_FAILED(rc)) {
        std::cerr << "Failed to initialize ndsp: " << std::hex << rc << std::endl;
        return false;
    }
    
    // Setup output mode
    ndspSetOutputMode(NDSP_OUTPUT_STEREO);
    
    audioInitialized = true;
    std::cout << "3DS audio system initialized successfully" << std::endl;
    return true;
}

void Audio::cleanup() {
    if (!audioInitialized) return;
    
    std::cout << "Cleaning up 3DS audio system..." << std::endl;
    
    // Stop all audio and clear buffers
    ndspChnReset(AUDIO_CHANNEL);
    ndspChnWaveBufClear(AUDIO_CHANNEL);
    
    // Free all audio tracks
    for (auto& track : audioTracks) {
        if (track.buffer) {
            linearFree(track.buffer);
        }
        if (track.buffer2) {
            linearFree(track.buffer2);
        }
    }
    audioTracks.clear();
    
    ndspExit();
    audioInitialized = false;
    std::cout << "3DS audio system cleaned up" << std::endl;
}

int Audio::loadWAV(const void* data, size_t size) {
    if (!audioInitialized) {
        std::cerr << "Audio not initialized" << std::endl;
        return -1;
    }
    
    std::cout << "Loading WAV file (" << size << " bytes)..." << std::endl;
    
    if (size < 44) {
        std::cerr << "Invalid WAV file - too small" << std::endl;
        return -1;
    }
    
    const char* wavData = (const char*)data;
    
    // Check for WAV signature
    if (memcmp(wavData, "RIFF", 4) != 0 || memcmp(wavData + 8, "WAVE", 4) != 0) {
        std::cerr << "Invalid WAV file - not a RIFF/WAVE file" << std::endl;
        return -1;
    }
    
    // Parse WAV header (simplified approach)
    uint16_t audioFormat = *((uint16_t*)(wavData + 20));
    uint16_t numChannels = *((uint16_t*)(wavData + 22));
    uint32_t sampleRate = *((uint32_t*)(wavData + 24));
    uint16_t bitsPerSample = *((uint16_t*)(wavData + 34));
    
    std::cout << "WAV format: " << audioFormat << ", channels: " << numChannels 
              << ", rate: " << sampleRate << ", bits: " << bitsPerSample << std::endl;
    
    if (audioFormat != 1) {
        std::cerr << "Unsupported WAV format: " << audioFormat << " (only PCM supported)" << std::endl;
        return -1;
    }
    
    if (bitsPerSample != 16) {
        std::cerr << "Unsupported bit depth: " << bitsPerSample << " (only 16-bit supported)" << std::endl;
        return -1;
    }
    
    if (numChannels < 1 || numChannels > 2) {
        std::cerr << "Unsupported channel count: " << numChannels << std::endl;
        return -1;
    }
    
    // Find audio data (skip to data after basic header)
    const char* audioData = wavData + 44;
    size_t audioSize = size - 44;
    
    std::cout << "Found audio data: " << audioSize << " bytes" << std::endl;
    
    // Calculate buffer size needed - use actual audio size for short sounds
    size_t bufferSize = std::max(audioSize, (size_t)(BUFFER_SIZE * sizeof(int16_t)));
    
    // Allocate buffers (double buffering for potential streaming)
    void* buffer1 = linearAlloc(bufferSize);
    void* buffer2 = linearAlloc(bufferSize);
    
    if (!buffer1 || !buffer2) {
        std::cerr << "Failed to allocate audio buffers (" << bufferSize << " bytes each)" << std::endl;
        if (buffer1) linearFree(buffer1);
        if (buffer2) linearFree(buffer2);
        return -1;
    }
    
    // Copy the entire audio data to first buffer
    memcpy(buffer1, audioData, audioSize);
    
    // Calculate actual samples
    size_t totalSamples = audioSize / sizeof(int16_t);
    size_t samplesPerChannel = totalSamples / numChannels;
    
    std::cout << "Total samples: " << totalSamples << ", samples per channel: " << samplesPerChannel << std::endl;
    
    // Create audio track
    AudioTrack track;
    track.buffer = buffer1;
    track.buffer2 = buffer2;
    track.audioData = audioData;
    track.size = audioSize;  // Actual audio data size
    track.bufferSize = bufferSize;  // Allocated buffer size
    track.isPlaying = false;
    track.channel = AUDIO_CHANNEL;
    track.numChannels = numChannels;
    track.sampleRate = sampleRate;
    track.bitsPerSample = bitsPerSample;
    track.currentPos = 0;
    track.samplesPerChannel = samplesPerChannel;
    
    audioTracks.push_back(track);
    int trackId = audioTracks.size() - 1;
    
    std::cout << "WAV loaded successfully as track " << trackId << std::endl;
    return trackId;
}

void Audio::playTrack(int trackId, bool loop) {
    if (!audioInitialized || trackId < 0 || trackId >= (int)audioTracks.size()) {
        std::cerr << "Invalid track ID: " << trackId << std::endl;
        return;
    }
    
    AudioTrack& track = audioTracks[trackId];
    if (track.isPlaying) {
        std::cout << "Track " << trackId << " is already playing" << std::endl;
        return;
    }
    
    std::cout << "Playing track " << trackId << "..." << std::endl;
    std::cout << "Audio data: " << track.size << " bytes, " << track.samplesPerChannel << " samples per channel" << std::endl;
    
    // Reset and clear channel
    ndspChnReset(AUDIO_CHANNEL);
    ndspChnWaveBufClear(AUDIO_CHANNEL);
    
    // Setup channel with proper parameters
    ndspChnSetInterp(AUDIO_CHANNEL, NDSP_INTERP_LINEAR);
    ndspChnSetRate(AUDIO_CHANNEL, track.sampleRate);
    
    // Set format based on track properties
    if (track.numChannels == 1) {
        ndspChnSetFormat(AUDIO_CHANNEL, NDSP_FORMAT_MONO_PCM16);
    } else {
        ndspChnSetFormat(AUDIO_CHANNEL, NDSP_FORMAT_STEREO_PCM16);
    }
    
    // Initialize wave buffers
    ndspWaveBuf* waveBuffers = (ndspWaveBuf*)track.waveBuf;
    memset(waveBuffers, 0, sizeof(ndspWaveBuf) * 2);
    
    // Set up buffer with the actual audio data
    waveBuffers[0].nsamples = track.samplesPerChannel;
    waveBuffers[0].data_vaddr = track.buffer;
    waveBuffers[0].looping = loop;
    
    std::cout << "Playing " << waveBuffers[0].nsamples << " samples per channel on channel " << AUDIO_CHANNEL << std::endl;
    
    // Flush cache with actual audio data size
    DSP_FlushDataCache(track.buffer, track.size);
    ndspChnWaveBufAdd(AUDIO_CHANNEL, &waveBuffers[0]);
    
    // Wait for playback to start
    while (!ndspChnIsPlaying(AUDIO_CHANNEL)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    track.isPlaying = true;
    track.currentPos = 0;  // Reset position
    
    std::cout << "Audio track " << trackId << " started successfully" << std::endl;
}

void Audio::stopTrack(int trackId) {
    if (!audioInitialized || trackId < 0 || trackId >= (int)audioTracks.size()) {
        return;
    }
    
    AudioTrack& track = audioTracks[trackId];
    if (!track.isPlaying) {
        return;
    }
    
    std::cout << "Stopping track " << trackId << std::endl;
    
    ndspChnReset(AUDIO_CHANNEL);
    ndspChnWaveBufClear(AUDIO_CHANNEL);
    track.isPlaying = false;
    track.currentPos = 0;
}

void Audio::stopAllTracks() {
    if (!audioInitialized) return;
    
    std::cout << "Stopping all audio tracks" << std::endl;
    
    ndspChnReset(AUDIO_CHANNEL);
    ndspChnWaveBufClear(AUDIO_CHANNEL);
    
    for (auto& track : audioTracks) {
        track.isPlaying = false;
        track.currentPos = 0;
    }
}

bool Audio::isTrackPlaying(int trackId) {
    if (!audioInitialized || trackId < 0 || trackId >= (int)audioTracks.size()) {
        return false;
    }
    
    AudioTrack& track = audioTracks[trackId];
    if (!track.isPlaying) {
        return false;
    }
    
    // Check if channel is still playing
    bool stillPlaying = ndspChnIsPlaying(AUDIO_CHANNEL);
    if (!stillPlaying) {
        std::cout << "Track " << trackId << " finished playing" << std::endl;
        track.isPlaying = false;
        track.currentPos = 0;
    }
    
    return stillPlaying;
}

void Audio::update() {
    if (!audioInitialized) return;
    
    // Update track status
    for (size_t i = 0; i < audioTracks.size(); i++) {
        AudioTrack& track = audioTracks[i];
        if (track.isPlaying) {
            if (!ndspChnIsPlaying(AUDIO_CHANNEL)) {
                track.isPlaying = false;
                track.currentPos = 0;
            }
        }
    }
} 