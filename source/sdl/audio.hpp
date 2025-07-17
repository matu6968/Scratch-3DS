#pragma once
#include <cstddef>

class Audio {
public:
    static bool init();
    static void cleanup();
    static int loadWAV(const void* data, size_t size);
    static void playTrack(int trackId, bool loop = false);
    static void stopTrack(int trackId);
    static void stopAllTracks();
    static bool isTrackPlaying(int trackId);
    static void update();
    
private:
    static bool initialized;
    static int nextTrackId;
    static const int MAX_TRACKS = 24;
}; 