#include <iostream>
#include <chrono>

#include "GameWindow.h"
#include "StateWindow.h"
#include "NameTableWindow.h"
#include "PatternTableWindow.h"
#include "SpritesWindow.h"
#include "PalettesWindow.h"

#include "NES.h"

int main(int argc, char* argv[]) {
    std::string romPath(argv[1]);
    std::string romDir;
    std::string romName;
    std::string gameName;

    size_t slashPosition = romPath.find_last_of("/");
    romDir = romPath.substr(0, slashPosition + 1);
    romName = romPath.substr(slashPosition + 1);
    std::cout << "Loading rom " << romPath << std::endl;

    NES nes(romPath);
    
    StateWindow stateWindow(nes);
    NameTableWindow nameTableWindow(nes);
    PatternTableWindow patternTableWindow(nes);
    SpritesWindow spritesWindow(nes);
    PalettesWindow palettesWindow(nes);

	std::vector<Window*> debugWindows;
    debugWindows.push_back(&stateWindow);
    debugWindows.push_back(&nameTableWindow);
    debugWindows.push_back(&patternTableWindow);
    debugWindows.push_back(&spritesWindow);
    debugWindows.push_back(&palettesWindow);

    GameWindow gameWindow(256, 240, romName, 0, 0, true, true);
    gameWindow.Update();
    nes.ppu.gameWindow = &gameWindow;

    nes.Reset();

    u16 framesCount = 0;
    auto previousFPSTime = std::chrono::system_clock::now();
    while (gameWindow.IsOpen()) {
        nes.DoInstruction();
        if (nes.frameFinished) {
            nes.frameFinished = false;
            gameWindow.Update();

            for (auto it = debugWindows.begin(); it != debugWindows.end(); it++)
                (*it)->Update();

            framesCount++;
            auto currentTime = std::chrono::system_clock::now();
            if (std::chrono::duration_cast<std::chrono::seconds>(currentTime - previousFPSTime).count() >= 1) {
                printf("FPS: %d\n", framesCount);
                previousFPSTime = currentTime;
                framesCount = 0;
                nes.DumpLogs();
            }
        }

        sf::Event event;
        while (gameWindow.PollEvent(event)) {
            if ((event.type == sf::Event::Closed) || ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::Escape)))
                gameWindow.Close();
            else if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Num1)
                    nameTableWindow.Toggle();
                else if (event.key.code == sf::Keyboard::Num2)
                    patternTableWindow.Toggle();
                else if (event.key.code == sf::Keyboard::Num3)
                    spritesWindow.Toggle();
                else if (event.key.code == sf::Keyboard::Num4)
                    stateWindow.Toggle();
                else if (event.key.code == sf::Keyboard::Num5)
                    palettesWindow.Toggle();
            }
        }

        for (auto it = debugWindows.begin(); it != debugWindows.end(); it++)
            (*it)->ProcessEvents();
    }

    for (auto it = debugWindows.begin(); it != debugWindows.end(); it++)
        (*it)->Close();
    debugWindows.clear();

	return 0;
}