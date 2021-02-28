#pragma once

#include <SFML\System.hpp>
#include <SFML\Graphics.hpp>
#include <string>
#include <memory>
#include "Types.h"

class Window {
public:
    Window(unsigned int Width, unsigned int Height, const std::string& Title, const int XPosition, const int YPosition, bool Open = true, bool scale = true);
	virtual ~Window();

    bool IsOpen() const;
    void Open();
    void Close();
    void Toggle();
    void GetFocus();

    void SetPosition(const int newXPosition, const int newYPosition);
    void SetTitle(const std::string& newTitle);

    bool PollEvent(sf::Event& event);
    virtual void ProcessEvents();
    void Update();

protected:
    virtual void RenderContent();

    unsigned width = 0;
    unsigned height = 0;
    std::string title;

    int xPosition;
    int yPosition;

    sf::Uint8* screenArray = nullptr;
    sf::Texture screenTexture;
    sf::Sprite screenSprite;

    sf::RenderWindow* renderWindow;

	bool scale = true;

private:
    void Initialize();
};
