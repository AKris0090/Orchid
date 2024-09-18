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

	Animation* activeAnimation;
	Animation* previousAnimation;
	bool needsSmooth;
	std::chrono::time_point<std::chrono::system_clock> smoothStart;
	std::chrono::time_point<std::chrono::system_clock> smoothUntil;
	std::chrono::milliseconds smoothDuration;
	float smoothAmount;
	float timeAdditional;
	int numInverseBindMatrices;
	std::vector<secondaryTransform>* src;
	std::vector<secondaryTransform>* dst;
	Transform transform;
	AnimatedGLTFObj* renderTarget;
	bool isDynamic;
	bool isOutline;
	bool isPlayerObj;
	DeviceHelper* pDevHelper;

	std::vector<Vertex> basePoseVertices_;

	VkBuffer vertexBuffer_;
	VkDeviceMemory vertexBufferMemory_;

	physx::PxRigidActor* physicsActor;
	physx::PxShape* pShape_;

	AnimatedGameObject(DeviceHelper* pD) { isDynamic = false; isPlayerObj = false; this->pDevHelper = pD; numInverseBindMatrices = 0; };

	void smoothFromCurrentPosition(std::vector<glm::mat4>& bindMatrices, float deltaTime);
	void updateAnimation(std::vector<glm::mat4>& bindMatrices, float deltaTime);
	void setAnimatedGLTFObj(AnimatedGLTFObj* obj) { this->renderTarget = obj; };

	void setTransform(glm::mat4 newTransform) { this->renderTarget->localModelTransform = newTransform; };
	void loopUpdate() {
		setTransform(transform.to_matrix());
	}

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
	static void getAnimatedNodeTransform(std::vector<AnimatedGameObject::secondaryTransform>* transforms, Animation* anim);
};;
