#pragma once

#include <SDL2/SDL_video.h>
#include <SDL2/SDL_events.h>


class IEventProcessor {
public:
	virtual void processEvent(const SDL_Event& event) = 0;
};

class MyWindow {

	SDL_Window* _window = nullptr;
	void* _ctx = nullptr;

	unsigned short _width = 800;
	unsigned short _height = 600;

public:
	int width() const { return _width; }
	int height() const { return _height; }
	double aspect() const { return static_cast<double>(_width) / _height; }

	float cameraDistance = 5.0f; // Distancia inicial de la cámara (zoom)
	float cameraAngleX = 0.0f; // Ángulo de rotación en el eje X
	float cameraAngleY = 0.0f; // Ángulo de rotación en el eje Y
	float panX = 0.0f, panY = 0.0f; // Desplazamiento de la cámara (pan)

	auto* windowPtr() const { return _window; }
	auto* contextPtr() const { return _ctx; }

	MyWindow(const char* title, unsigned short width, unsigned short height);
	MyWindow(MyWindow&&) noexcept = delete;
	MyWindow(const MyWindow&) = delete;
	MyWindow& operator=(const MyWindow&) = delete;
	~MyWindow();

	void open(const char* title, unsigned short width, unsigned short height);
	void close();
	bool isOpen() const { return _window; }

	bool processEvents(IEventProcessor* event_processor = nullptr);
	void draw();
	void swapBuffers() const;

};