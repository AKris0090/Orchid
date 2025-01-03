#pragma once

#include "MeshHelper.h"
#include <filesystem>

struct AnimSceneNode {
	AnimSceneNode* parent;
	uint32_t index;
	std::vector<AnimSceneNode*> children;
	std::vector<MeshHelper*> meshPrimitives;
	glm::vec3 position{ 1.0f };
	glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
	glm::vec3 scale{ 1.0f };
	glm::mat4 matrix = glm::mat4(1.0f);
	glm::mat4 getAnimatedMatrix() const;

	int32_t skinIndex = -1;
};

class Animation {
public:
	struct AnimationGraph {

	} animGraph;

	struct AnimationSampler {
		std::string interpolation;
		std::vector<float> inputs;
		std::vector<glm::vec4> outputsVec4;
	};

	struct AnimationChannel {
		std::string path;
		AnimSceneNode* node;
		uint32_t samplerIndex;
	};

	int numChannels;
	std::string name;
	std::vector<AnimationSampler> samplers;
	std::vector<AnimationChannel> channels;
	float start = std::numeric_limits<float>::max();
	float end = std::numeric_limits<float>::min();
	float currentTime = 0.0f;

	void loadAnimation(std::string gltfPath_, std::vector<AnimSceneNode*> pParentNodes);
};
