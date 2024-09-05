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

	static float weightLerp(float a, float b, float t) {
		return (a * (1.0f - t)) + (b * (t));
	}

	static glm::vec3 weightLerp(glm::vec3 a, glm::vec3 b, float t) {
		return (a * (1.0f - t)) + (b * (t));
	}

	std::chrono::time_point<std::chrono::system_clock> getCurrentTime();
	float getDeltaTime();
};