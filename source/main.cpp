#include <chrono>
#include <thread>
#include "scratch/blockExecutor.hpp"
#include "scratch/render.hpp"
#include "scratch/input.hpp"
#include "scratch/unzip.hpp"
#ifdef __3DS__
#include "3ds/audio.hpp"
#else
#include "sdl/audio.hpp"
#endif

// arm-none-eabi-addr2line -e Scratch.elf xxx
// ^ for debug purposes

static void exitApp(){
	Audio::cleanup();
	Render::deInit();
}

static void initApp(){
	Render::Init();
	Audio::init();
}

int main(int argc, char **argv)
{
	initApp();
	
	// this is for the FPS
	std::chrono::_V2::system_clock::time_point startTime = std::chrono::high_resolution_clock::now();
	std::chrono::_V2::system_clock::time_point endTime = std::chrono::high_resolution_clock::now();
	//this is for frametime check
	std::chrono::_V2::system_clock::time_point frameStartTime = std::chrono::high_resolution_clock::now();
	std::chrono::_V2::system_clock::time_point frameEndTime = std::chrono::high_resolution_clock::now();

	if(!Unzip::load()){
		exitApp();
		return 0;
	}

	BlockExecutor::timer = std::chrono::high_resolution_clock::now();
	bool flagClickedExecuted = false;

	while (Render::appShouldRun())
	{
		
		endTime = std::chrono::high_resolution_clock::now();
		if(endTime - startTime >= std::chrono::milliseconds(1000 / Scratch::FPS)){
			startTime = std::chrono::high_resolution_clock::now();
			frameStartTime = std::chrono::high_resolution_clock::now();

			Input::getInput();
			BlockExecutor::runRepeatBlocks();
			Audio::update();
			Render::renderSprites();
			
			// Execute flag clicked blocks after first frame is rendered
			if (!flagClickedExecuted) {
				BlockExecutor::runAllBlocksByOpcode(Block::EVENT_WHENFLAGCLICKED);
				flagClickedExecuted = true;
			}

			frameEndTime = std::chrono::high_resolution_clock::now();
			auto frameDuration = frameEndTime - frameStartTime;
			//std::cout << "\x1b[17;1HFrame time: " << frameDuration.count() << " ms" << std::endl;
			//std::cout << "\x1b[18;1HSprites: " << sprites.size() << std::endl;
			
		}
		if(toExit){
			break;
		}
	}
	

	exitApp();
	return 0;
}