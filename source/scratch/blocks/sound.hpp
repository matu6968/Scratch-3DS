#pragma once
#include "../blockExecutor.hpp"
#include "../image.hpp"

class SoundBlocks{
public:
    static Value volume(Block& block, Sprite* sprite);
    static BlockResult playUntilDone(Block& block, Sprite* sprite, Block** waitingBlock, bool* withoutScreenRefresh);
    static BlockResult play(Block& block, Sprite* sprite, Block** waitingBlock, bool* withoutScreenRefresh);
    static BlockResult stopAllSounds(Block& block, Sprite* sprite, Block** waitingBlock, bool* withoutScreenRefresh);
    static BlockResult changeEffectBy(Block& block, Sprite* sprite, Block** waitingBlock, bool* withoutScreenRefresh);
    static BlockResult setEffectTo(Block& block, Sprite* sprite, Block** waitingBlock, bool* withoutScreenRefresh);
    static BlockResult clearEffects(Block& block, Sprite* sprite, Block** waitingBlock, bool* withoutScreenRefresh);
    static BlockResult changeVolumeBy(Block& block, Sprite* sprite, Block** waitingBlock, bool* withoutScreenRefresh);
    static BlockResult setVolumeTo(Block& block, Sprite* sprite, Block** waitingBlock, bool* withoutScreenRefresh);
    static Value soundsMenu(Block& block, Sprite* sprite);
    
    // Sound loading from archives (declared in cpp file)
    static void loadSounds(void *zip);
    
private:
    static void playSound(const std::string& soundName, Sprite* sprite);
    static void stopAllPlayingSounds();
    static bool isSoundPlaying(const std::string& soundName);
    static int loadSoundFile(Sound* sound);
};