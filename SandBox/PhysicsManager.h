#pragma once

#include <PxPhysics.h>
#include <PxPhysicsAPI.h>
#include <PxCooking.h>
#include <iostream>
#include <vector>
#include "MeshHelper.h"
#include "GameObject.h"

class PhysicsManager {
private:
	bool recordMemoryAllocations = true;
	physx::PxDefaultErrorCallback defaultErrorCallback;
	physx::PxDefaultAllocator defaultAllocatorCallback;
	physx::PxDefaultCpuDispatcher* pDispatcher;

	physx::PxFoundation* pFoundation_ = NULL;
	physx::PxPhysics* pPhysics_ = NULL;

	physx::PxScene* pScene = NULL;
	physx::PxMaterial* pMaterial = NULL;

	physx::PxPvd* pPVirtDebug = NULL;

	physx::PxTolerancesScale pTolerancesScale_;

public:
	PhysicsManager() {};

	physx::PxShape* createPhysicsFromMesh(MeshHelper* mesh, physx::PxMaterial* material, glm::vec3 scale);
	void addShapes(MeshHelper* mesh, std::vector<GameObject*> gameObjects);

	void setup();
	void loopUpdate(std::vector<GameObject*> gameObjects);
	void shutDown();
};