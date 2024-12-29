#pragma once

#include "DirectionalLight.h"
#include "AnimatedGLTFObj.h"

class AnimatedGameObject {
public:
	struct secondaryTransform {
		glm::vec3 position;
		glm::quat rotation;
		glm::vec3 scale;
	};

	Transform transform;
	std::vector<AnimatedGLTFObj*> renderTargets;
	void addRenderTarget(AnimatedGLTFObj* newObj) { 
		renderTargets.push_back(newObj);
	};
	bool isDynamic = false;
	bool isOutline;
	bool isPlayerObj;
	DeviceHelper* pDevHelper;

	physx::PxRigidActor* physicsActor;
	physx::PxShape* pShape_;

	bool needsSmooth;
	std::chrono::milliseconds smoothDuration;
	std::chrono::time_point<std::chrono::system_clock> smoothStart;
	std::chrono::time_point<std::chrono::system_clock> smoothUntil;
	float timeAdditional;

	Animation* activeAnimation;
	Animation* previousAnimation;
	float smoothAmount;

	AnimatedGameObject() {};
	~AnimatedGameObject() {
		for (auto& g : renderTargets) {
			if (g) {
				delete g;
			}
		}

		pShape_->release();
		physicsActor->release();
	};
	AnimatedGameObject(DeviceHelper* pD) { isDynamic = false; isPlayerObj = false; this->pDevHelper = pD; };

	void smoothFromCurrentPosition(std::vector<glm::mat4>& bindMatrices, float deltaTime);
	void updateAnimation(std::vector<glm::mat4>& bindMatrices, float deltaTime);

	void loopUpdate() {
		scriptUpdate();
		transform.matrix = transform.to_matrix();
	}
	virtual void scriptUpdate() {};

private:
	enum STRINGENUM {
		TRANSLATION,
		ROTATION,
		SCALE
	};

	STRINGENUM hash_str(std::string const& inString) {
		if (inString == "translation") return TRANSLATION;
		if (inString == "rotation") return ROTATION;
		if (inString == "scale") return SCALE;
	}

	glm::mat4 getNodeMatrix(AnimSceneNode* node);
	void updateJoints(AnimSceneNode* node, std::vector<glm::mat4>& bindMatrices);
	static void getAnimatedNodeTransform(std::vector<AnimatedGLTFObj::secondaryTransform>* transforms, Animation* anim);
};;
