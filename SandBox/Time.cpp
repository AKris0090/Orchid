#include "Time.h"

namespace Time {
	std::chrono::time_point<std::chrono::system_clock> lastTime;;
	std::chrono::time_point<std::chrono::system_clock> currentTime;
	float deltaTime;
	bool start = false;

	void Time::updateTime() {
		currentTime = std::chrono::system_clock::now();

		if (!start) {
			lastTime = currentTime;
			deltaTime = 0;
			start = true;
		}
		else {
			std::chrono::duration<float> elapsed_seconds = currentTime - lastTime;
			deltaTime = elapsed_seconds.count();
			lastTime = currentTime;
		}
	}

	float Time::getDeltaTime() {
		return deltaTime;
	}
}