#include "Window.h"

Window::Window(unsigned int Width, unsigned int Height, const std::string& Title, const int XPosition, const int YPosition, bool Open, bool scale)
    : width(Width), height(Height), title(Title), xPosition(XPosition), yPosition(YPosition), scale(scale) {
	screenArray = new sf::Uint32[width * height];
    memset(screenArray, 0, width * height * 4);

	screenTexture.create(width, height);
	screenTexture.update((sf::Uint8*)screenArray);

	screenSprite.setTexture(screenTexture, true);
	screenSprite.setPosition(0.0f, 0.0f);

    renderWindow = new sf::RenderWindow(sf::VideoMode(width, height), title);
    Initialize();

    if (!Open)
        Close();
}

Window::~Window() {
    Close();
    delete[] screenArray;
    screenArray = nullptr;
    delete renderWindow;
    renderWindow = nullptr;
}

void Window::Toggle() {
    if (IsOpen())
        Close();
    else
        Open();
}

void Window::Open() {
    if (!renderWindow->isOpen()) {
        renderWindow->create(sf::VideoMode(width, height), title);
        Initialize();
    }
}

void Window::Close() {
    if (renderWindow->isOpen())
        renderWindow->close();
}

bool Window::IsOpen() const {
    return renderWindow->isOpen();
}

void Window::SetPosition(const int NewXPosition, const int NewYPosition) {
    xPosition = NewXPosition;
    yPosition = NewYPosition;
    renderWindow->setPosition({ xPosition, yPosition });
}

void Window::GetFocus() {
    renderWindow->requestFocus();
}

bool Window::PollEvent(sf::Event& event) {
    return renderWindow->pollEvent(event);
}

void Window::ProcessEvents() {
    sf::Event event;
    while (PollEvent(event))
        if ((event.type == sf::Event::Closed) || ((event.type == sf::Event::KeyPressed) && (event.key.code == sf::Keyboard::Escape)))
            Close();
}

void Window::Initialize() {
    renderWindow->setPosition({ xPosition, yPosition });
	if (scale) {
		sf::Vector2u s(width * 2, height * 2);
		renderWindow->setSize(s);
	}
    renderWindow->setVerticalSyncEnabled(false);
}

void Window::SetTitle(const std::string& newTitle) {
    renderWindow->setTitle(newTitle);
}

void Window::Update() {
    if (!IsOpen())
        return;

    renderWindow->clear();
    RenderContent();
    renderWindow->display();
}

void Window::RenderContent() {
    screenTexture.update((sf::Uint8*)screenArray);
    screenSprite.setTexture(screenTexture, true);
    renderWindow->draw(screenSprite);
}