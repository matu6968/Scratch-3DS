#include "sensing.hpp"
#include "../input.hpp"
#include "../keyboard.hpp"

BlockResult SensingBlocks::resetTimer(Block& block, Sprite* sprite, Block** waitingBlock, bool* withoutScreenRefresh) {
    BlockExecutor::timer = std::chrono::high_resolution_clock::now();
    return BlockResult::CONTINUE;
}

BlockResult SensingBlocks::askAndWait(Block& block, Sprite* sprite, Block** waitingBlock, bool* withoutScreenRefresh) {
    Keyboard kbd;
    Value inputValue = Scratch::getInputValue(block,"QUESTION",sprite);
    std::string output = kbd.openKeyboard(inputValue.asString().c_str());
    answer = output;
    return BlockResult::CONTINUE;
}

BlockResult SensingBlocks::setDragMode(Block& block, Sprite* sprite, Block** waitingBlock, bool* withoutScreenRefresh){

    std::string mode = block.fields.at("DRAG_MODE")[0];

    if(mode == "draggable"){
        sprite->draggable = true;
    }
    else if(mode == "not draggable"){
        sprite->draggable = false;
    }


    return BlockResult::CONTINUE;
}

Value SensingBlocks::sensingTimer(Block& block, Sprite* sprite) {
    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - BlockExecutor::timer).count();
    return Value(elapsed);
}

Value SensingBlocks::of(Block& block, Sprite* sprite) {
    std::string value = block.fields.at("PROPERTY")[0];
    std::string object;
    auto objectFind = block.parsedInputs.find("OBJECT");
    Block* objectBlock = findBlock(objectFind->second.literalValue.asString());
    if(!objectBlock || objectBlock == nullptr)
        return Value();
    
    object = objectBlock->fields.at("OBJECT")[0];

    Sprite* spriteObject = nullptr;
    for (Sprite* currentSprite : sprites) {
        if (currentSprite->name == object && !currentSprite->isClone) {
            spriteObject = currentSprite;
            break;
        }
    }
    
    if (!spriteObject) return Value(0);
    
    if (value == "timer") {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - BlockExecutor::timer).count();
        return Value(elapsed);
    } else if (value == "x position") {
        return Value(spriteObject->xPosition);
    } else if (value == "y position") {
        return Value(spriteObject->yPosition);
    } else if (value == "direction") {
        return Value(spriteObject->rotation);
    } else if (value == "costume #" || value == "backdrop #") {
        return Value(spriteObject->currentCostume + 1);
    } else if (value == "costume name" || value == "backdrop name") {
        return Value(spriteObject->costumes[spriteObject->currentCostume].name);
    } else if (value == "size") {
        return Value(spriteObject->size);
    } else if (value == "volume") {
        return Value(spriteObject->volume);
    }
    
    for (const auto& [id, variable] : spriteObject->variables) {
        if (value == variable.name) {
            return variable.value;
        }
    }
    return Value(0);
}

Value SensingBlocks::mouseX(Block& block, Sprite* sprite) {
    return Value(Input::mousePointer.x);
}

Value SensingBlocks::mouseY(Block& block, Sprite* sprite) {
    return Value(Input::mousePointer.y);
}

Value SensingBlocks::distanceTo(Block& block, Sprite* sprite) {
    auto inputFind = block.parsedInputs.find("DISTANCETOMENU");
    Block* inputBlock = findBlock(inputFind->second.literalValue.asString());
    std::string object = inputBlock->fields.at("DISTANCETOMENU")[0];
    
    if (object == "_mouse_") {
        return Value(sqrt(pow(Input::mousePointer.x - sprite->xPosition, 2) + 
                                 pow(Input::mousePointer.y - sprite->yPosition, 2)));
    }
    
    for (Sprite* currentSprite : sprites) {
        if (currentSprite->name == object && !currentSprite->isClone) {
            double distance = sqrt(pow(currentSprite->xPosition - sprite->xPosition, 2) + 
                                 pow(currentSprite->yPosition - sprite->yPosition, 2));
            return Value(distance);
        }
    }
    return Value(0);
}

Value SensingBlocks::daysSince2000(Block& block, Sprite* sprite) {
    return Value(Time::getDaysSince2000());
}

Value SensingBlocks::current(Block& block, Sprite* sprite) {
    std::string inputValue;
    try {
        inputValue = block.fields.at("CURRENTMENU")[0];
    } catch (...) {
        return Value();
    }
    
    if (inputValue == "YEAR") return Value(Time::getYear());
    if (inputValue == "MONTH") return Value(Time::getMonth());
    if (inputValue == "DATE") return Value(Time::getDay());
    if (inputValue == "DAYOFWEEK") return Value(Time::getDayOfWeek());
    if (inputValue == "HOUR") return Value(Time::getHours());
    if (inputValue == "MINUTE") return Value(Time::getMinutes());
    if (inputValue == "SECOND") return Value(Time::getSeconds());
    
    return Value();
}

Value SensingBlocks::sensingAnswer(Block& block, Sprite* sprite) {
    return Value(answer);
}

Value SensingBlocks::keyPressed(Block& block, Sprite* sprite){
    auto inputFind = block.parsedInputs.find("KEY_OPTION");
    Block* inputBlock = findBlock(inputFind->second.literalValue.asString());
    for (std::string button : Input::inputButtons) {
        if (inputBlock->fields["KEY_OPTION"][0] == button) {
            return Value(true);
        }
    }
    return Value(false);
}

Value SensingBlocks::touchingObject(Block& block, Sprite* sprite){
    auto inputFind = block.parsedInputs.find("TOUCHINGOBJECTMENU");
    Block* inputBlock = findBlock(inputFind->second.literalValue.asString());
    std::string objectName;
    try {
        objectName = inputBlock->fields["TOUCHINGOBJECTMENU"][0];
    } catch (...) {
        return Value(false);
    }

    // Get collision points of the current sprite
    std::vector<std::pair<double, double>> currentSpritePoints = getCollisionPoints(sprite);

    if(objectName == "_mouse_") {
        // Check if the mouse pointer's position is within the bounds of the current sprite
        if (Input::mousePointer.x >= sprite->xPosition - sprite->spriteWidth / 2 &&
            Input::mousePointer.x <= sprite->xPosition + sprite->spriteWidth / 2 &&
            Input::mousePointer.y >= sprite->yPosition - sprite->spriteHeight / 2 &&
            Input::mousePointer.y <= sprite->yPosition + sprite->spriteHeight / 2) {
            return Value(true);
        }
        return Value(false);
    }

    if (objectName == "_edge_") {
        double halfWidth = Scratch::projectWidth / 2.0;
        double halfHeight = Scratch::projectHeight / 2.0;

        // Check if the current sprite is touching the edge of the screen
        if (sprite->xPosition <= -halfWidth || sprite->xPosition >= halfWidth ||
            sprite->yPosition <= -halfHeight || sprite->yPosition >= halfHeight) {
            return Value(true);
        }
        return Value(false);
    }

    for (Sprite* targetSprite : sprites) {
        if (targetSprite->name == objectName && targetSprite->visible) {
            // Get collision points of the target sprite
            std::vector<std::pair<double, double>> targetSpritePoints = getCollisionPoints(targetSprite);

            // Check if any point of the current sprite is inside the target sprite's bounds
            for (const auto& point : currentSpritePoints) {
                if (point.first >= targetSprite->xPosition - targetSprite->spriteWidth / 2 &&
                    point.first <= targetSprite->xPosition + targetSprite->spriteWidth / 2 &&
                    point.second >= targetSprite->yPosition - targetSprite->spriteHeight / 2 &&
                    point.second <= targetSprite->yPosition + targetSprite->spriteHeight / 2) {
                    return Value(true);
                }
            }

            // Check if any point of the target sprite is inside the current sprite's bounds
            for (const auto& point : targetSpritePoints) {
                if (point.first >= sprite->xPosition - sprite->spriteWidth / 2 &&
                    point.first <= sprite->xPosition + sprite->spriteWidth / 2 &&
                    point.second >= sprite->yPosition - sprite->spriteHeight / 2 &&
                    point.second <= sprite->yPosition + sprite->spriteHeight / 2) {
                    return Value(true);
                }
            }
        }
    }
    return Value(false);
}

Value SensingBlocks::mouseDown(Block& block, Sprite* sprite){
    return Value(Input::mousePointer.isPressed);
}

Value SensingBlocks::username(Block& block, Sprite* sprite){
    return Value(Input::getUsername());
}