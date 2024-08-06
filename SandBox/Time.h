#pragma once

#include <chrono>
#include <glm/glm.hpp>

namespace Time {
	void updateTime();

	static float lerp(float a, float b, float t) {
		return a + ((b-a) * t);
	}

	static glm::vec3 lerp(glm::vec3 a, glm::vec3 b, float t) {
		return a + ((b - a) * t);
	}

	std::chrono::time_point<std::chrono::system_clock> getCurrentTime();
	float getDeltaTime();
};