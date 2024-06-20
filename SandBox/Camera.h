#pragma once

#include <SDL.h>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include "DeviceHelper.h"
#include <physx/PxPhysicsAPI.h>
#include <physx/PxPhysics.h>

class FPSCam { // TEMPLATE FROM VKGUIDE
private:
	glm::vec3 velocity_ = { 0.0f, 0.0f, 0.0f };
	// up-down
	float pitch_ = 0.0f;
	// left-right
	float yaw_ = 0.0f;
	glm::mat4 getRotationMatrix();
	float nearPlane;
	float farPlane;
	float aspectRatio;
	float FOV;

public:
	float moveSpeed_ = 0.005f; // slow is 0.0015;
	glm::mat4 viewMatrix;
	glm::mat4 inverseViewMatrix;
	glm::mat4 rotationMatrix;
	glm::vec3 right;
	glm::vec3 forward;
	glm::vec3 up;
	float distanceToPlayer;
	bool isAttatched;

	inline glm::vec3 PxVec3toGlmVec3(physx::PxVec3 vec) {
		return { vec.x, vec.y, vec.z };
	};

	inline glm::vec3 PxVec3toGlmVec3(physx::PxExtendedVec3 vec) {
		return { vec.x, vec.y, vec.z };
	};

	inline physx::PxVec3 GlmVec3tpPxVec3(glm::vec3 vec) {
		return { vec.x, vec.y, vec.z };
	};

	Transform transform;

	FPSCam() { distanceToPlayer = 1.75f; isAttatched = true; };

	void update(Transform playerTransform);
	void baseUpdate();
	void alterUpdate(Transform playerTransform, float capHeight);
	void physicsUpdate(Transform playerTransform, physx::PxScene* scene, physx::PxController* characterController, float capsuleHeight);
	void setVelocity(glm::vec3 vel);
	void setPosition(glm::vec3 pos);
	void processSDL(SDL_Event& e);
	void setPitchYaw(float nPitch, float nYaw);
	glm::mat4 getViewMatrix();
	void setNearPlane(float nearP);
	void setFarPlane(float farP);
	void setAspectRatio(float aspect);
	void setFOV(float fov);
	float getNearPlane() { return this->nearPlane; };
	float getFarPlane() { return this->farPlane; };
	float getAspectRatio() { return this->aspectRatio; };
	float getFOV() { return this->FOV; };
};