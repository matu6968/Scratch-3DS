#include "audio.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <iostream>
#include <unordered_map>
#include <vector>

// Static member definitions
bool Audio::initialized = false;
int Audio::nextTrackId = 0;

// Internal data structures
static std::unordered_map<int, Mix_Chunk*> loadedSounds;
static std::unordered_map<int, int> trackChannels; // Maps track ID to channel
static std::vector<bool> channelInUse(24, false); // Using 24 directly instead of MAX_TRACKS

bool Audio::init() {
    if (initialized) {
        return true;
    }
    
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cout << "SDL audio init failed: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Initialize SDL_mixer
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cout << "SDL_mixer init failed: " << Mix_GetError() << std::endl;
        return false;
    }
    
    // Allocate channels
    Mix_AllocateChannels(MAX_TRACKS);
    
    initialized = true;
    std::cout << "SDL audio initialized with " << MAX_TRACKS << " channels" << std::endl;
    return true;
}

void Audio::cleanup() {
    if (!initialized) {
        return;
    }
    
    // Stop all sounds
    Mix_HaltChannel(-1);
    
    // Free all loaded sounds
    for (auto& [trackId, chunk] : loadedSounds) {
        Mix_FreeChunk(chunk);
    }
    loadedSounds.clear();
    trackChannels.clear();
    
    // Close SDL_mixer
    Mix_CloseAudio();
    
    initialized = false;
    std::cout << "SDL audio cleaned up" << std::endl;
}

int Audio::loadWAV(const void* data, size_t size) {
    if (!initialized) {
        std::cout << "Audio not initialized" << std::endl;
        return -1;
    }
    
    // Create SDL_RWops from memory buffer
    SDL_RWops* rw = SDL_RWFromConstMem(data, size);
    if (!rw) {
        std::cout << "Failed to create SDL_RWops: " << SDL_GetError() << std::endl;
        return -1;
    }
    
    // Load WAV from memory
    Mix_Chunk* chunk = Mix_LoadWAV_RW(rw, 1); // 1 = free RW automatically
    if (!chunk) {
        std::cout << "Failed to load WAV: " << Mix_GetError() << std::endl;
        return -1;
    }
    
    // Store the sound with a unique track ID
    int trackId = nextTrackId++;
    loadedSounds[trackId] = chunk;
    
    std::cout << "Loaded WAV as track " << trackId << std::endl;
    return trackId;
}

void Audio::playTrack(int trackId, bool loop) {
    if (!initialized) {
        std::cout << "Audio not initialized" << std::endl;
        return;
    }
    
    auto it = loadedSounds.find(trackId);
    if (it == loadedSounds.end()) {
        std::cout << "Track not found: " << trackId << std::endl;
        return;
    }
    
    // Find available channel
    int channel = -1;
    for (int i = 0; i < MAX_TRACKS; i++) {
        if (!channelInUse[i]) {
            channel = i;
            break;
        }
    }
    
    if (channel == -1) {
        std::cout << "No available channels" << std::endl;
        return;
    }
    
    // Play the sound
    int result = Mix_PlayChannel(channel, it->second, loop ? -1 : 0);
    if (result == -1) {
        std::cout << "Failed to play sound: " << Mix_GetError() << std::endl;
        return;
    }
    
    // Track the channel usage
    channelInUse[channel] = true;
    trackChannels[trackId] = channel;
    
    std::cout << "Playing track " << trackId << " on channel " << channel << std::endl;
}

void Audio::stopTrack(int trackId) {
    if (!initialized) {
        return;
    }
    
    auto it = trackChannels.find(trackId);
    if (it != trackChannels.end()) {
        int channel = it->second;
        Mix_HaltChannel(channel);
        channelInUse[channel] = false;
        trackChannels.erase(it);
        std::cout << "Stopped track " << trackId << " on channel " << channel << std::endl;
    }
}

void Audio::stopAllTracks() {
    if (!initialized) {
        return;
    }
    
    Mix_HaltChannel(-1);
    
    // Reset all channel usage
    for (int i = 0; i < MAX_TRACKS; i++) {
        channelInUse[i] = false;
    }
    trackChannels.clear();
    
    std::cout << "Stopped all tracks" << std::endl;
}

bool Audio::isTrackPlaying(int trackId) {
    if (!initialized) {
        return false;
    }
    
    auto it = trackChannels.find(trackId);
    if (it != trackChannels.end()) {
        int channel = it->second;
        return Mix_Playing(channel) != 0;
    }
    
    return false;
}

void Audio::update() {
    if (!initialized) {
        return;
    }
    
    // Update channel usage based on actual playing status
    for (auto it = trackChannels.begin(); it != trackChannels.end(); ) {
        int channel = it->second;
        
        if (Mix_Playing(channel) == 0) {
            // Channel is no longer playing
            channelInUse[channel] = false;
            it = trackChannels.erase(it);
        } else {
            ++it;
        }
    }
} 