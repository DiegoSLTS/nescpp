#include "Window.h"
#include "StateWindow.h"

#include "Memory.h"
#include "CPU.h"

int main(int argc, char* argv) {
    Memory memory;
    CPU cpu(memory);

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