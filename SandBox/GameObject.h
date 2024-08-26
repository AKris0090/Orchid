#pragma once

#include "DirectionalLight.h"

class GameObject {
private:

public:
	Transform transform;
	GLTFObj* renderTarget;
	bool isDynamic;
	bool isOutline;

	physx::PxRigidActor* physicsActor;
	physx::PxShape* pShape_;

	GameObject() {
		isDynamic = false;
	};

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

	void setGLTFObj(GLTFObj* obj) { this->renderTarget = obj; };
	void setTransform(glm::mat4 newTransform) { this->renderTarget->localModelTransform = newTransform; };

	void loopUpdate() {
		setTransform(transform.to_matrix());
	}
};;