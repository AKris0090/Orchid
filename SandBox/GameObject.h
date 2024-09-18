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

	void setGLTFObj(GLTFObj* obj) { this->renderTarget = obj; };
	void setTransform(glm::mat4 newTransform) { this->renderTarget->localModelTransform = newTransform; };

	void loopUpdate() {
		setTransform(transform.to_matrix());
	}
};;