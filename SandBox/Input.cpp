#include "Input.h"

namespace Input {
	bool forward;
	bool backward;
	bool left;
	bool right;
	bool mouseButton;

	void handleSDLInput(SDL_Event& e) {
		if (e.type == SDL_KEYDOWN) {
			if (e.key.keysym.sym == SDLK_w) { forward = true; }

			if (e.key.keysym.sym == SDLK_s) { backward = true; }

			if (e.key.keysym.sym == SDLK_a) { left = true; }

			if (e.key.keysym.sym == SDLK_d) { right = true; }
		}

		if (e.type == SDL_MOUSEBUTTONDOWN) {
			if (e.button.button == SDL_BUTTON_LEFT) {
				mouseButton = true;
			}
		}

		if (e.type == SDL_KEYUP) {
			if (e.key.keysym.sym == SDLK_w) { forward = false; }

			if (e.key.keysym.sym == SDLK_s) { backward = false; }

			if (e.key.keysym.sym == SDLK_a) { left = false; }

			if (e.key.keysym.sym == SDLK_d) { right = false; }
		}


		if (e.type == SDL_MOUSEBUTTONUP) {
			if (e.button.button == SDL_BUTTON_LEFT) {
				mouseButton = false;
			}
		}
	}

	bool forwardKeyDown() {
		return forward;
	}

	bool backwardKeyDown() {
		return backward;
	}

	bool rightKeyDown() {
		return right;
	}

	bool leftKeyDown() {
		return left;
	}

	bool leftMouseDown() {
		return mouseButton;
	}
}