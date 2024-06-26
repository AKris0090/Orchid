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
		MeshHelper* mesh;
		glm::vec3 translation{};
		glm::vec3 scale{1.0f};
		glm::quat rotation{};
		glm::mat4 matrix;
		glm::mat4 getAnimatedMatrix();

		int32_t skinIndex = -1;
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

	struct cascadeMVP {
		glm::mat4 model;
		uint32_t cascadeIndex;
	} cascadeBlock;

	struct pcBlock {
		int alphaMask;
		float alphaCutoff;
	} pushConstantBlock;

	std::vector<SceneNode*> pParentNodes;
	glm::mat4 modelTransform;
	std::unordered_map<Material*, std::vector<SceneNode*>> opaqueDraws;
	std::unordered_map<Material*, std::vector<SceneNode*>> transparentDraws;
	std::vector<Skin> skins_;

	std::vector<int32_t> textureIndices_;
	std::vector<TextureHelper*> images_;
	std::vector<Material> mats_;

	AnimatedGLTFObj(std::string gltfPath, DeviceHelper* deviceHelper);
	void loadGLTF(uint32_t globalVertexOffset, uint32_t globalIndexOffset);

	void createDescriptors(VkDescriptorSetLayout descSetLayout);

	void drawIndexedOpaque(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
	void drawIndexedTransparent(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
	void renderShadow(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VkPipeline animatedShadowPipeline, uint32_t cascadeIndex, VkDescriptorSet cascadeDescriptor);

	uint32_t getTotalVertices() { return this->totalVertices_; };
	uint32_t getTotalIndices() { return this->totalIndices_; };

	uint32_t activeAnimation;
	void updateAnimation(float deltaTime);

private:
	std::string gltfPath_;
	DeviceHelper* pDevHelper_;
	uint32_t totalIndices_;
	uint32_t totalVertices_;

	tinygltf::Model* pInputModel_;

	std::vector<Animation> animations_;

	void loadImages();
	void loadTextures();
	void loadMaterials();
	void loadSkins();
	void loadAnimations(tinygltf::Model& in, std::vector<Animation>& animations);
	void updateJoints(SceneNode* node);
	glm::mat4 getNodeMatrix(SceneNode* node);
	void loadNode(tinygltf::Model& in, const tinygltf::Node& nodeIn, uint32_t index, SceneNode* parent, std::vector<SceneNode*>& nodes, uint32_t globalVertexOffset, uint32_t globalIndexOffset);
	void drawShadow(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VkPipeline animatedShadowPipeline, uint32_t cascadeIndex, VkDescriptorSet cascadeDescriptor, SceneNode* node);
	SceneNode* nodeFromIndex(uint32_t index);
	SceneNode* findNode(AnimatedGLTFObj::SceneNode* parent, uint32_t index);
	void callIndexedDraw(VkCommandBuffer& commandBuffer, MeshHelper::indirectDrawInfo& indexedDrawInfo);
};;
