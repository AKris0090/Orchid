#pragma once

#include <SDL.h>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

class FPSCam { // TEMPLATE FROM VKGUIDE
private:
	glm::vec3 velocity_ = { 0.0f, 0.0f, 0.0f };
	// up-down
	float pitch_ = 0.0f;
	// left-right
	float yaw_ = 0.0f;
	glm::mat4 getRotationMatrix();

public:
	float moveSpeed_ = 0.005f; // slow is 0.0015;
	glm::vec3 position_ = { 0.0f, 0.0f, 0.0f };
	glm::vec4 viewPos_ = { 0.0f, 0.0f, 0.0f, 0.0f };

	void update();
	void setVelocity(glm::vec3 vel);
	void setPosition(glm::vec3 pos);
	void processSDL(SDL_Event& e);
	void setPitchYaw(float nPitch, float nYaw);
	glm::mat4 getViewMatrix();
};