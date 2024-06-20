#pragma once

#include "MeshHelper.h"
#include "Camera.h"
#include "Input.h"
#include "AnimatedGameObject.h"

#define PLAYER_CAP_HEIGHT 0.85f;
#define PLAYER_CAP_RADIUS 0.15f;

class PlayerObject {
private:
	physx::PxControllerManager* manager;
	physx::PxCapsuleControllerDesc desc;

	physx::PxScene* pScene_;
	physx::PxMaterial* pMaterial_;

public:
	float cap_height = PLAYER_CAP_HEIGHT;
	float cap_radius = PLAYER_CAP_RADIUS;
	physx::PxController* characterController;
	float playerSpeed = 0.008f;
	bool isWalking;
	AnimatedGameObject* playerGameObject;

	inline glm::vec3 PxVec3toGlmVec3(physx::PxExtendedVec3 vec) {
		return { vec.x, vec.y, vec.z };
	}

	MeshHelper* playerMesh;
	Transform transform;

	PlayerObject(physx::PxMaterial* material, physx::PxScene* pScene);
	void setupPhysicsController();
	void loopUpdate(FPSCam* camera);
};