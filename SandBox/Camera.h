#pragma once

#include <SDL.h>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

class FPSCam { // TEMPLATE FROM VKGUIDE
private:
	glm::vec3 velocity = { 0.0f, 0.0f, 0.0f };

	// up-down
	float pitch = 0.0f;

	// left-right
	float yaw = 0.0f;

	glm::mat4 getRotationMatrix();
public:
	glm::vec3 position = { 0.0f, 0.0f, 0.0f };
	glm::vec4 viewPos = { 0.0f, 0.0f, 0.0f, 0.0f };

	void update();
	void setVelocity(glm::vec3 vel);
	void setPosition(glm::vec3 pos);
	void processSDL(SDL_Event& e);

	void setPitchYaw(float nPitch, float nYaw);
	glm::mat4 getViewMatrix();
};