#pragma once

#include <vector>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>
#include "MeshHelper.h"
#include "mikktspace.h"

class AnimatedGLTFObj {
public:
	struct SceneNode {
		SceneNode* parent;
		uint32_t index;
		std::vector<SceneNode*> children;
		MeshHelper::MeshObj mesh;
		glm::vec3 translation{};
		glm::vec3 scale{1.0f};
		glm::quat rotation{};
		int32_t skin = -1;
		glm::mat4 matrix;
		glm::mat4 getLocalMatrix();
	};

	struct Skin {
		std::string name;
		SceneNode* skeletonRoot = nullptr;
		std::vector<glm::mat4> inverseBindMatrices;
		std::vector<SceneNode*> joints;
		VkBuffer ssboBufferHandle;
		void* ssbo;
		VkDescriptorSet descriptorSet;
		VkDescriptorSet shadowMapDescriptorSet;
	};

	struct AnimationSampler {
		std::string interpolation;
		std::vector<float> inputs;
		std::vector<glm::vec4> outputsVec4;
	};

	struct AnimationChannel {
		std::string path;
		SceneNode* node;
		uint32_t samplerIndex;
	};

	struct Animation {
		std::string name;
		std::vector<AnimationSampler> samplers;
		std::vector<AnimationChannel> channels;
		float start = std::numeric_limits<float>::max();
		float end = std::numeric_limits<float>::min();
		float currentTime = 0.0f;
	};

	struct depthMVModel {
		glm::mat4 mvp;
		glm::mat4 model;
	} depthPushBlock_;

	MeshHelper* pSceneMesh_;

	glm::mat4 modelTransform;
	glm::vec3* pos;

	AnimatedGLTFObj(std::string gltfPath, DeviceHelper* deviceHelper);
	void loadGLTF();

	void createDescriptors(VkDescriptorSetLayout descSetLayout);

	void render(VkCommandBuffer commandBuffer, VkPipelineLayout& pipelineLayout);
	void renderShadow(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VkPipeline animatedShadowPipeline, glm::mat4 mvp);

	uint32_t getTotalVertices() { return this->totalVertices_; };
	uint32_t getTotalIndices() { return this->totalIndices_; };
	MeshHelper* getMeshHelper() { return pSceneMesh_; };
	uint32_t activeAnimation;
	void updateAnimation(float deltaTime);

private:
	std::string gltfPath_;
	DeviceHelper* pDevHelper_;
	uint32_t totalIndices_;
	uint32_t totalVertices_;
	tinygltf::Model* pInputModel_;
	std::vector<SceneNode*> pNodes_;

	std::vector<Skin> skins_;
	std::vector<Animation> animations_;

	void loadImages(tinygltf::Model& in, std::vector<TextureHelper*>& images);
	void loadTextures(tinygltf::Model& in, std::vector<TextureHelper::TextureIndexHolder>& textures);
	void loadMaterials(tinygltf::Model& in, std::vector<MeshHelper::Material>& mats);
	void loadSkins(tinygltf::Model& in, std::vector<Skin>& skins);
	void loadAnimations(tinygltf::Model& in, std::vector<Animation>& animations);
	void updateJoints(SceneNode* node);
	glm::mat4 getNodeMatrix(SceneNode* node);
	void loadNode(tinygltf::Model& in, const tinygltf::Node& nodeIn, uint32_t index, SceneNode* parent, std::vector<SceneNode*>& nodes);
	void drawIndexed(VkCommandBuffer commandBuffer, VkPipelineLayout& pipelineLayout, SceneNode* pNode);
	void drawShadow(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VkPipeline animatedShadowPipeline, SceneNode* node);
	SceneNode* nodeFromIndex(uint32_t index);
	SceneNode* findNode(AnimatedGLTFObj::SceneNode* parent, uint32_t index);
};
