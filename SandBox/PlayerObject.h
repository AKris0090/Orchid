#pragma once

#include "MeshHelper.h"

#define PLAYER_CAP_HEIGHT 0.4f;
#define PLAYER_CAP_RADIUS 0.15f;

class PlayerObject {
private:
	physx::PxControllerManager* manager;
	physx::PxCapsuleControllerDesc desc;

	physx::PxScene* pScene_;
	physx::PxMaterial* pMaterial_;

public:
	float cap_height = 0.4f;
	float cap_radius = 0.15f;
	physx::PxController* characterController;
	float playerSpeed = 0.0085f;

	MeshHelper* playerMesh;

	PlayerObject(physx::PxMaterial* material, physx::PxScene* pScene);
	void setupPhysicsController();
};