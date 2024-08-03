#pragma once

#include <iostream>
#include <vector>
#include "GameObject.h"
#include "AnimatedGameObject.h"
#include "Input.h"
#include "GLTFObject.h"
#include "PlayerObject.h"

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

	std::vector<physx::PxShape*> createPhysicsFromMesh(GameObject* g, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, physx::PxMaterial* material, glm::vec3& scale);
	void addCubeToGameObject(GameObject* gameObject, physx::PxVec3 globalTransform, float halfExtent);
	void addPlane();
	void addShapeToGameObject(GameObject* gameObject, physx::PxVec3 globalTransform, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, glm::vec3& scale);
	void recursiveAddToList(GameObject* g, std::vector<physx::PxVec3>& pxVertices, std::vector<uint32_t>& pxIndices, GLTFObj::SceneNode* node, std::vector<Vertex>& vertices);

	void setup();
	void loopUpdate(AnimatedGameObject* playerAnimObject, std::vector<GameObject*> gameObjects, std::vector<AnimatedGameObject*> animatedGameObjects, PlayerObject* player, FPSCam* cam, float deltaTime);
	void shutDown();
};