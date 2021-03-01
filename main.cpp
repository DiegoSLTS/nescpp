#include <iostream>

#include "Window.h"
#include "StateWindow.h"

#include "Cartridge.h"
#include "CPU.h"
#include "Memory.h"

int main(int argc, char* argv[]) {
    std::string romPath;
    std::string romDir;
    std::string romName;
    std::string gameName;

    romPath = argv[1];
    size_t slashPosition = romPath.find_last_of("/");
    romDir = romPath.substr(0, slashPosition + 1);
    romName = romPath.substr(slashPosition + 1);
    std::cout << "Loading rom " << romPath << std::endl;

    Cartridge cart(romPath);
    Memory memory(&cart);
    CPU cpu(memory);
    memory.cpu = &cpu;

    StateWindow stateWindow(cpu);
	std::vector<Window*> debugWindows;
    debugWindows.push_back(&stateWindow);

    Window mainWindow(160, 140, "Test", 0, 0);
    mainWindow.Update();

    while (mainWindow.IsOpen())
    {
        sf::Event event;
        while (mainWindow.PollEvent(event)) {
            if ((event.type == sf::Event::Closed) || ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::Escape)))
                mainWindow.Close();
            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Space) {
                    cpu.Update();
                    mainWindow.Update();
                    
                    for (auto it = debugWindows.begin(); it != debugWindows.end(); it++)
                        (*it)->Update();
                }
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