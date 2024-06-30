#pragma once

#include "DirectionalLight.h"
#include <PxPhysics.h>
#include <PxPhysicsAPI.h>
#include <PxCooking.h>
#include "AnimatedGLTFObj.h"

class AnimatedGameObject {
private:

public:
	Transform transform;
	AnimatedGLTFObj* renderTarget;
	bool isDynamic;
	bool isPlayerObj;
	DeviceHelper* pDevHelper;
	VkDevice device_;

	std::vector<Vertex> basePoseVertices_;
	std::vector<uint32_t> basePoseIndices_;

	VkBuffer vertexBuffer_;
	VkDeviceMemory vertexBufferMemory_;

	VkBuffer indexBuffer_;
	VkDeviceMemory indexBufferMemory_;

	VkBuffer skinnedBuffer_;
	VkDeviceMemory skinnedBufferDeviceMemory_;

	physx::PxRigidActor* physicsActor;
	physx::PxShape* pShape_;

	AnimatedGameObject(DeviceHelper* pD) { isDynamic = false; isPlayerObj = false; this->pDevHelper = pD; this->device_ = pD->getDevice(); };

	void createVertexBuffer();
	void createSkinnedBuffer();
	void createIndexBuffer();

	glm::mat4 toGLMMat4(physx::PxMat44 pxMatrix) {
		glm::mat4 matrix = glm::mat4(1.0f);
		for (int x = 0; x < 4; x++)
			for (int y = 0; y < 4; y++)
				matrix[x][y] = pxMatrix[x][y];
		return matrix;
	}

	glm::vec3 PxVec3toGlmVec3(physx::PxVec3 pxVec) {
		glm::vec3 vector = glm::vec3(pxVec.x, pxVec.y, pxVec.z);
		return vector;
	}

	void updateAnimation(std::vector<glm::mat4>& bindMatrices, float deltaTime);
	glm::mat4 getNodeMatrix(AnimatedGLTFObj::SceneNode* node);
	void updateJoints(AnimatedGLTFObj::SceneNode* node, std::vector<glm::mat4>& bindMatrices);

	void setAnimatedGLTFObj(AnimatedGLTFObj* obj) { this->renderTarget = obj; };
	void setTransform(glm::mat4 newTransform) { this->renderTarget->modelTransform = newTransform; };

	void loopUpdate() {
		setTransform(transform.to_matrix());
	}
};;
