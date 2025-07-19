#include "mp3.hpp"
#include <mpg123.h>
#include <cstdio>
#include <cstring>

// MP3 decoder wrapper for our audio system
class MP3Decoder {
private:
    mpg123_handle* mh;
    uint32_t sampleRate;
    uint8_t channels;
    uint16_t bitsPerSample;
    size_t bufferSize;
    
public:
    MP3Decoder() : mh(nullptr), sampleRate(0), channels(0), bitsPerSample(16), bufferSize(0) {}
    
    ~MP3Decoder() {
        cleanup();
    }
    
    bool init(const void* data, size_t size) {
        int err = 0;
        int encoding = 0;
        
        // Initialize libmpg123
        if ((err = mpg123_init()) != MPG123_OK) {
            printf("mpg123_init failed: %s\n", mpg123_plain_strerror(err));
            return false;
        }
        
        // Create new handle
        if ((mh = mpg123_new(nullptr, &err)) == nullptr) {
            printf("mpg123_new failed: %s\n", mpg123_plain_strerror(err));
            return false;
        }
        
        // Open from memory
        if (mpg123_open_feed(mh) != MPG123_OK) {
            printf("mpg123_open_feed failed: %s\n", mpg123_strerror(mh));
            return false;
        }
        
        // Feed the data
        if (mpg123_feed(mh, (const unsigned char*)data, size) != MPG123_OK) {
            printf("mpg123_feed failed: %s\n", mpg123_strerror(mh));
            return false;
        }
        
        // Get format information
        long rate;
        int chans;
        if (mpg123_getformat(mh, &rate, &chans, &encoding) != MPG123_OK) {
            printf("mpg123_getformat failed: %s\n", mpg123_strerror(mh));
            return false;
        }
        
        sampleRate = (uint32_t)rate;
        channels = (uint8_t)chans;
        bitsPerSample = 16; // mpg123 outputs 16-bit samples
        
        // Ensure this output format will not change
        mpg123_format_none(mh);
        mpg123_format(mh, rate, chans, encoding);
        
        // Calculate buffer size
        bufferSize = mpg123_outblock(mh) * 8; // Similar to ctrmus
        
        printf("MP3 initialized: %luHz, %d channels, %d bits\n", (unsigned long)sampleRate, (int)channels, bitsPerSample);
        
        return true;
    }
    
    void cleanup() {
        if (mh) {
            mpg123_close(mh);
            mpg123_delete(mh);
            mh = nullptr;
        }
        mpg123_exit();
    }
    
    size_t decode(void* buffer, size_t maxSize) {
        if (!mh) return 0;
        
        size_t done = 0;
        int result = mpg123_read(mh, (unsigned char*)buffer, maxSize, &done);
        
        if (result == MPG123_DONE) {
            return 0; // End of file
        } else if (result != MPG123_OK && result != MPG123_NEW_FORMAT) {
            printf("mpg123_read failed: %s\n", mpg123_strerror(mh));
            return 0;
        }
        
        return done;
    }
    
    uint32_t getSampleRate() const { return sampleRate; }
    uint8_t getChannels() const { return channels; }
    uint16_t getBitsPerSample() const { return bitsPerSample; }
    size_t getBufferSize() const { return bufferSize; }
};

// C-style interface for integration with our Audio system
static MP3Decoder* currentDecoder = nullptr;

bool MP3::init(const void* data, size_t size) {
    if (currentDecoder) {
        delete currentDecoder;
    }
    
    currentDecoder = new MP3Decoder();
    return currentDecoder->init(data, size);
}

void MP3::cleanup() {
    if (currentDecoder) {
        delete currentDecoder;
        currentDecoder = nullptr;
    }
}

size_t MP3::decode(void* buffer, size_t maxSize) {
    if (!currentDecoder) return 0;
    return currentDecoder->decode(buffer, maxSize);
}

uint32_t MP3::getSampleRate() {
    if (!currentDecoder) return 0;
    return currentDecoder->getSampleRate();
}

uint8_t MP3::getChannels() {
    if (!currentDecoder) return 0;
    return currentDecoder->getChannels();
}

uint16_t MP3::getBitsPerSample() {
    if (!currentDecoder) return 0;
    return currentDecoder->getBitsPerSample();
}

size_t MP3::getBufferSize() {
    if (!currentDecoder) return 0;
    return currentDecoder->getBufferSize();
} 