#pragma once

#include <iostream>
#include <vector>
#include "PlayerObject.h"
#include "GameObject.h"
#include "AnimatedGameObject.h"
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

	PhysicsManager() {};

	physx::PxShape* createPhysicsFromMesh(MeshHelper* mesh, physx::PxMaterial* material, glm::vec3 scale);
	void addCubeToGameObject(GameObject* gameObject, physx::PxVec3 globalTransform, float halfExtent);
	void addShapeToGameObject(GameObject* gameObject, physx::PxVec3 globalTransform, glm::vec3 scale);

	void setup();
	void loopUpdate(AnimatedGameObject* playerAnimObject, std::vector<GameObject*> gameObjects, std::vector<AnimatedGameObject*> animatedGameObjects, PlayerObject* player, FPSCam* cam, float deltaTime);
	void shutDown();
};