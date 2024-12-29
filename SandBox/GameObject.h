#pragma once

#include "DirectionalLight.h"

class GameObject {
private:

public:
	GameObject() {};
	~GameObject() {
		for (auto& g : renderTargets) {
			if (g) {
				delete g;
			}
		}
		for (auto& t : renderTargetTransforms) {
			delete t;
		}

		pShape_->release();
		physicsActor->release();
	};
	std::vector<Transform*> renderTargetTransforms;
	std::vector<GLTFObj*> renderTargets;
	bool isDynamic = false;

	physx::PxRigidActor* physicsActor;
	physx::PxShape* pShape_;

	void addRenderTarget(GLTFObj* newObj) { 
		renderTargets.push_back(newObj); 
		renderTargetTransforms.push_back(new Transform());
	};
	void loopUpdate() {
		scriptUpdate();
		for (int i = 0; i < renderTargets.size(); i++) {
			renderTargetTransforms[i]->matrix = renderTargetTransforms[i]->to_matrix();
		}
	};
	virtual void scriptUpdate() {};
};