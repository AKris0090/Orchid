#pragma once

#include <iostream>
#include <vector>
#include "PlayerObject.h"
#include "GameObject.h"
#include "AnimatedGameObject.h"
#include "Camera.h"
#include "Input.h"

class PhysicsManager {
private:
	bool recordMemoryAllocations = true;
	physx::PxDefaultErrorCallback defaultErrorCallback;
	physx::PxDefaultAllocator defaultAllocatorCallback;
	physx::PxDefaultCpuDispatcher* pDispatcher;

	physx::PxFoundation* pFoundation_ = NULL;
	physx::PxPhysics* pPhysics_ = NULL;

	physx::PxPvd* pPVirtDebug = NULL;

	physx::PxTolerancesScale pTolerancesScale_;

public:
	physx::PxMaterial* pMaterial = NULL;
	physx::PxScene* pScene = NULL;

	glm::vec3 playerGlobalDisplacement;

	inline glm::vec3 PxVec3toGlmVec3(physx::PxExtendedVec3 vec) {
		return { vec.x, vec.y, vec.z };
	}


	PhysicsManager() {};

	physx::PxShape* createPhysicsFromMesh(MeshHelper* mesh, physx::PxMaterial* material, glm::vec3 scale);
	void addCubeToGameObject(GameObject* gameObject, physx::PxVec3 globalTransform, float halfExtent);
	void addShapeToGameObject(GameObject* gameObject, physx::PxVec3 globalTransform, glm::vec3 scale);
	void createCharacterController(PlayerObject* player);

	void setup();
	void loopUpdate(std::vector<GameObject*> gameObjects, PlayerObject* player, FPSCam* cam, float deltaTime);
	void shutDown();
};