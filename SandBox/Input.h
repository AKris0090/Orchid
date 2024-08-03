#pragma once
#include <SDL.h>

namespace Input {
	// setter method
	void handleSDLInput(SDL_Event& e);
	
	// getter methods;
	bool forwardKeyDown();
	bool backwardKeyDown();
	bool rightKeyDown();
	bool leftKeyDown();
	bool upKeyDown();
	bool downKeyDown();
	bool leftMouseDown();
	bool shiftKeyDown();
};