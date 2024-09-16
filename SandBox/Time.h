#pragma once

#include <chrono>
#include <glm/glm.hpp>

constexpr auto DIFFPI = 3.141592653589793;

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

	static float newSine(float val) {
		return ((glm::sin((DIFFPI * val) - (DIFFPI / 2.0f)) + 1) / 2.0f);
	}

	static float smoothStep(float low, float high, float current) {
		float height = high - low;
		return (newSine(current) * height - low);
	}

	std::chrono::time_point<std::chrono::system_clock> getCurrentTime();
	float getDeltaTime();
};