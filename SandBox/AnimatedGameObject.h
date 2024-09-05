#pragma once

#include "DirectionalLight.h"
#include "AnimatedGLTFObj.h"

class AnimatedGameObject {
private:

public:
	Animation* activeAnimation;
	Animation* previousAnimation;
	bool needsSmooth;
	std::chrono::time_point<std::chrono::system_clock> smoothStart;
	std::chrono::time_point<std::chrono::system_clock> smoothUntil;
	std::chrono::milliseconds smoothDuration;
	float smoothAmount;

	Transform transform;
	AnimatedGLTFObj* renderTarget;
	bool isDynamic;
	bool isOutline;
	bool isPlayerObj;
	DeviceHelper* pDevHelper;

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

	AnimatedGameObject(DeviceHelper* pD) { isDynamic = false; isPlayerObj = false; this->pDevHelper = pD; };

	void createVertexBuffer();

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

	void smoothFromCurrentPosition(std::vector<glm::mat4>& bindMatrices, float deltaTime);
	void updateAnimation(std::vector<glm::mat4>& bindMatrices, float deltaTime);
	glm::mat4 getNodeMatrix(AnimSceneNode* node);
	void updateJoints(AnimSceneNode* node, std::vector<glm::mat4>& bindMatrices);

	void setAnimatedGLTFObj(AnimatedGLTFObj* obj) { this->renderTarget = obj; };
	void setTransform(glm::mat4 newTransform) { this->renderTarget->localModelTransform = newTransform; };

	void loopUpdate() {
		setTransform(transform.to_matrix());
	}
};;
