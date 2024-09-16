#pragma once

#include "MeshHelper.h"
#include "Camera.h"
#include "Input.h"
#include "AnimatedGameObject.h"
#include "Time.h"

#define PLAYER_CAP_HEIGHT 0.85f;
#define PLAYER_CAP_RADIUS 0.15f;

enum PLAYERSTATE {
	IDLE,
	WALKING,
	RUNNING
};

class PlayerObject {
private:
	physx::PxControllerManager* manager;
	physx::PxCapsuleControllerDesc desc;

	float turnSpeed;
	physx::PxScene* pScene_;
	physx::PxMaterial* pMaterial_;

public:
	PLAYERSTATE previousState;
	PLAYERSTATE currentState;
	float cap_height = PLAYER_CAP_HEIGHT;
	float cap_radius = PLAYER_CAP_RADIUS;
	physx::PxController* characterController;
	float playerWalkSpeed = 0.0065f;
	float playerRunSpeed = 0.0130f;
	float currentSpeed = 0.0065f;
	AnimatedGameObject* playerGameObject;

	inline glm::vec3 PxVec3toGlmVec3(physx::PxExtendedVec3 vec) {
		return { vec.x, vec.y, vec.z };
	}

	MeshHelper* playerMesh;
	Transform transform;

	PlayerObject(physx::PxMaterial* material, physx::PxScene* pScene);

	void transitionState(PLAYERSTATE newState);
	void setupPhysicsController();
	void loopUpdate(FPSCam* camera);
};