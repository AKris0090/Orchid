#pragma once

#include <chrono>

namespace Time {
	void updateTime();

	static float lerp(float a, float b, float t) {
		return a + ((b-a) * t);
	}

	float getDeltaTime();
};