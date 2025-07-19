#include "sound.hpp"
#include "../interpret.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <fstream>
#include <cstring>
#include <algorithm>

// Platform-specific audio includes
#ifdef __3DS__
#include "../../3ds/audio.hpp"
#include <3ds.h>
#else
#include "../../sdl/audio.hpp"
#endif

// Include miniz for ZIP archive support
#include "miniz/miniz.h"

// Static variables for sound management
static std::vector<std::string> currentlyPlayingSounds;
static std::unordered_map<std::string, bool> soundPlayingStatus;
static std::unordered_map<std::string, int> soundTrackIds; // Maps sound names to track IDs

// Sound cache for archive loading
struct CachedSound {
    std::string filename;
    void* data;
    size_t size;
};
static std::unordered_map<std::string, CachedSound> cachedSounds;

Value SoundBlocks::volume(Block& block, Sprite* sprite) {
    return Value(sprite->volume);
}

BlockResult SoundBlocks::playUntilDone(Block& block, Sprite* sprite, Block** waitingBlock, bool* withoutScreenRefresh) {
    // Get sound name from SOUND_MENU input
    Value soundMenuValue = Scratch::getInputValue(block, "SOUND_MENU", sprite);
    std::string soundName = soundMenuValue.asString();
    
#ifdef __3DS__
    // Simpler approach for 3DS - just play sound and continue
    // Find the sound in the sprite's sounds
    auto soundIt = sprite->sounds.find(soundName);
    if (soundIt == sprite->sounds.end()) {
        // Try to find by name
        for (const auto& [id, sound] : sprite->sounds) {
            if (sound.name == soundName) {
                soundIt = sprite->sounds.find(id);
                break;
            }
        }
    }
    
    if (soundIt == sprite->sounds.end()) {
        std::cerr << "Sound not found: " << soundName << std::endl;
        return BlockResult::CONTINUE;
    }
    
    // Play the sound and continue (non-blocking)
    playSound(soundName, sprite);
    return BlockResult::CONTINUE;
#else
    // PC version - full asynchronous implementation
    // First execution - start playing sound
    if (block.repeatTimes == -1) {
        block.repeatTimes = -10; // Use unique flag for sound playback
        
        // Find the sound in the sprite's sounds
        auto soundIt = sprite->sounds.find(soundName);
        if (soundIt == sprite->sounds.end()) {
            // Try to find by name
            for (const auto& [id, sound] : sprite->sounds) {
                if (sound.name == soundName) {
                    soundIt = sprite->sounds.find(id);
                    break;
                }
            }
        }
        
        if (soundIt == sprite->sounds.end()) {
            std::cerr << "Sound not found: " << soundName << std::endl;
            return BlockResult::CONTINUE;
        }
        
        // Start playing the sound (non-blocking)
        playSound(soundName, sprite);
        
        // Store the sound name for tracking
        block.soundName = soundName;
        
        // Add to repeat queue to check completion
        BlockExecutor::addToRepeatQueue(sprite, const_cast<Block*>(&block));
    }
    
    // Check if sound is still playing
    if (!block.soundName.empty() && isSoundPlaying(block.soundName)) {
        return BlockResult::RETURN; // Keep waiting
    }
    
    // Sound finished - clean up and continue
    block.repeatTimes = -1;
    
    // Safely remove from repeat queue
    if (!block.blockChainID.empty() && 
        sprite->blockChains.find(block.blockChainID) != sprite->blockChains.end()) {
        auto& repeatList = sprite->blockChains[block.blockChainID].blocksToRepeat;
        if (!repeatList.empty()) {
            repeatList.pop_back();
        }
    }
    
    return BlockResult::CONTINUE;
#endif
}

BlockResult SoundBlocks::play(Block& block, Sprite* sprite, Block** waitingBlock, bool* withoutScreenRefresh) {
    // Get sound name from SOUND_MENU input
    Value soundMenuValue = Scratch::getInputValue(block, "SOUND_MENU", sprite);
    std::string soundName = soundMenuValue.asString();
    
    playSound(soundName, sprite);
    
    return BlockResult::CONTINUE;
}

BlockResult SoundBlocks::stopAllSounds(Block& block, Sprite* sprite, Block** waitingBlock, bool* withoutScreenRefresh) {
    stopAllPlayingSounds();
    return BlockResult::CONTINUE;
}

BlockResult SoundBlocks::changeEffectBy(Block& block, Sprite* sprite, Block** waitingBlock, bool* withoutScreenRefresh) {
    // TODO: Implement sound effects (pitch, pan, etc.)
    return BlockResult::CONTINUE;
}

BlockResult SoundBlocks::setEffectTo(Block& block, Sprite* sprite, Block** waitingBlock, bool* withoutScreenRefresh) {
    // TODO: Implement sound effects (pitch, pan, etc.)
    return BlockResult::CONTINUE;
}

BlockResult SoundBlocks::clearEffects(Block& block, Sprite* sprite, Block** waitingBlock, bool* withoutScreenRefresh) {
    // TODO: Implement clearing sound effects
    return BlockResult::CONTINUE;
}

BlockResult SoundBlocks::changeVolumeBy(Block& block, Sprite* sprite, Block** waitingBlock, bool* withoutScreenRefresh) {
    Value changeValue = Scratch::getInputValue(block, "VOLUME", sprite);
    sprite->volume += changeValue.asInt();
    if (sprite->volume < 0) sprite->volume = 0;
    if (sprite->volume > 100) sprite->volume = 100;
    return BlockResult::CONTINUE;
}

BlockResult SoundBlocks::setVolumeTo(Block& block, Sprite* sprite, Block** waitingBlock, bool* withoutScreenRefresh) {
    Value volumeValue = Scratch::getInputValue(block, "VOLUME", sprite);
    sprite->volume = volumeValue.asInt();
    if (sprite->volume < 0) sprite->volume = 0;
    if (sprite->volume > 100) sprite->volume = 100;
    return BlockResult::CONTINUE;
}

Value SoundBlocks::soundsMenu(Block& block, Sprite* sprite) {
    std::string soundName = block.fields["SOUND_MENU"][0].get<std::string>();
    return Value(soundName);
}

void SoundBlocks::playSound(const std::string& soundName, Sprite* sprite) {
    // Find the sound in the sprite's sounds
    Sound* soundToPlay = nullptr;
    
    for (const auto& [id, sound] : sprite->sounds) {
        if (sound.name == soundName || id == soundName) {
            soundToPlay = const_cast<Sound*>(&sound);
            break;
        }
    }
    
    if (!soundToPlay) {
        std::cerr << "Sound not found: " << soundName << std::endl;
        return;
    }
    
    std::cout << "Playing sound: " << soundName << " (format: " << soundToPlay->dataFormat << ")" << std::endl;
    
    // Check if sound is already loaded
    auto trackIt = soundTrackIds.find(soundName);
    int trackId = -1;
    
    if (trackIt != soundTrackIds.end()) {
        trackId = trackIt->second;
    } else {
        // Load the sound file
        trackId = loadSoundFile(soundToPlay);
        if (trackId >= 0) {
            soundTrackIds[soundName] = trackId;
        }
    }
    
    if (trackId < 0) {
        std::cerr << "Failed to load sound: " << soundName << std::endl;
        return;
    }
    
    // Platform-specific sound playing
    Audio::playTrack(trackId, false);
    
    // Track playing sounds
    currentlyPlayingSounds.push_back(soundName);
    soundPlayingStatus[soundName] = true;
}

int SoundBlocks::loadSoundFile(Sound* sound) {
    if (!sound) return -1;
    
    // Load the sound file from the project
    std::string filename = sound->fullName;
    
    // Read the file data
    void* fileData = nullptr;
    size_t fileSize = 0;
    
    if (projectType == UNZIPPED) {
        // Load from filesystem
#ifdef __3DS__
        std::string filepath = "romfs:/project/" + filename;
#else
        std::string filepath = "project/" + filename;
#endif
        
        FILE* file = fopen(filepath.c_str(), "rb");
        if (!file) {
            std::cerr << "Failed to open sound file: " << filepath << std::endl;
            return -1;
        }
        
        fseek(file, 0, SEEK_END);
        fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);
        
        fileData = malloc(fileSize);
        if (!fileData) {
            fclose(file);
            std::cerr << "Failed to allocate memory for sound file" << std::endl;
            return -1;
        }
        
        fread(fileData, 1, fileSize, file);
        fclose(file);
    } else {
        // Load from cached sound data (loaded from archive during project loading)
        auto cachedIt = cachedSounds.find(sound->id);
        if (cachedIt == cachedSounds.end()) {
            std::cerr << "Sound not found in cache: " << sound->id << std::endl;
            return -1;
        }
        
        const CachedSound& cachedSound = cachedIt->second;
        fileData = cachedSound.data;
        fileSize = cachedSound.size;
        
        std::cout << "Using cached sound: " << cachedSound.filename << " (" << fileSize << " bytes)" << std::endl;
    }
    
    if (!fileData || fileSize == 0) {
        std::cerr << "Failed to load sound file: " << filename << std::endl;
        return -1;
    }
    
    std::cout << "Loaded sound file: " << filename << " (" << fileSize << " bytes)" << std::endl;
    
    // Detect file format based on extension
    std::string extension = "";
    size_t lastDot = filename.find_last_of('.');
    if (lastDot != std::string::npos) {
        extension = filename.substr(lastDot);
        std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
    }
    
    // Load the sound into the audio system
    int trackId = -1;
    
    if (extension == ".wav") {
        trackId = Audio::loadWAV(fileData, fileSize);
    } else if (extension == ".mp3") {
#ifdef __3DS__
        // Use the MP3 decoder for 3DS
        trackId = Audio::loadMP3(fileData, fileSize);
#else
        // For PC, SDL can handle MP3 files through loadWAV
        trackId = Audio::loadWAV(fileData, fileSize);
#endif
    } else {
        std::cerr << "Unsupported audio format: " << extension << " for file: " << filename << std::endl;
        trackId = -1;
    }
    
    // Free the file data (audio system should have copied it)
    // Only free if it was loaded from filesystem, not from cache
    if (projectType == UNZIPPED) {
        free(fileData);
    }
    
    return trackId;
}

void SoundBlocks::stopAllPlayingSounds() {
    std::cout << "Stopping all sounds" << std::endl;
    
    Audio::stopAllTracks();
    
    currentlyPlayingSounds.clear();
    soundPlayingStatus.clear();
}

bool SoundBlocks::isSoundPlaying(const std::string& soundName) {
    auto trackIt = soundTrackIds.find(soundName);
    if (trackIt == soundTrackIds.end()) {
        return false;
    }
    
    return Audio::isTrackPlaying(trackIt->second);
}

void SoundBlocks::loadSounds(void *zip_ptr) {
    mz_zip_archive *zip = (mz_zip_archive *)zip_ptr;
    std::cout << "Loading sounds from archive..." << std::endl;
    int file_count = (int)mz_zip_reader_get_num_files(zip);

    for (int i = 0; i < file_count; i++) {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(zip, i, &file_stat)) continue;

        std::string zipFileName = file_stat.m_filename;

        // Check if file is a WAV or MP3 file
        if (zipFileName.size() >= 4 && 
            (zipFileName.substr(zipFileName.size() - 4) == ".wav" || 
             zipFileName.substr(zipFileName.size() - 4) == ".WAV" ||
             zipFileName.substr(zipFileName.size() - 4) == ".mp3" ||
             zipFileName.substr(zipFileName.size() - 4) == ".MP3")) {

            size_t file_size;
            void* file_data = mz_zip_reader_extract_to_heap(zip, i, &file_size, 0);
            if (!file_data) {
                std::cout << "Failed to extract sound: " << zipFileName << std::endl;
                continue;
            }

            // Create a copy of the data for caching
            void* cached_data = malloc(file_size);
            if (!cached_data) {
                std::cout << "Failed to allocate memory for cached sound: " << zipFileName << std::endl;
                mz_free(file_data);
                continue;
            }
            memcpy(cached_data, file_data, file_size);
            mz_free(file_data);

            // Cache the sound data
            CachedSound sound;
            sound.filename = zipFileName;
            sound.data = cached_data;
            sound.size = file_size;
            
            // Remove extension for the cache key
            std::string soundId = zipFileName.substr(0, zipFileName.find_last_of('.'));
            cachedSounds[soundId] = sound;
            
            std::cout << "Cached sound: " << zipFileName << " (" << file_size << " bytes)" << std::endl;
        }
    }
}