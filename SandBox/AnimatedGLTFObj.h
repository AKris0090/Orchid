#pragma once

#include <vector>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>
#include "Animation.h"
#include "mikktspace.h"

class AnimatedGLTFObj {
public:
	struct Skin {
		std::string name;
		AnimSceneNode* skeletonRoot = nullptr;
		std::vector<glm::mat4> inverseBindMatrices;
		std::vector<AnimSceneNode*> joints;
		VkDescriptorSet descriptorSet;
		VkDescriptorSet shadowMapDescriptorSet;
		std::vector<glm::mat4>* finalJointMatrices;
	};

	Animation anim;

	struct cascadeMVP {
		glm::mat4 model;
		uint32_t cascadeIndex;
	} cascadeBlock;

	struct pcBlock {
		int alphaMask;
		float alphaCutoff;
	} pushConstantBlock;

	std::vector<AnimSceneNode*> pParentNodes;
	glm::mat4 modelTransform;
	std::unordered_map<Material*, std::vector<MeshHelper*>> opaqueDraws;
	std::unordered_map<Material*, std::vector<MeshHelper*>> transparentDraws;
	std::vector<Skin> skins_;

	uint32_t globalSkinningMatrixOffset;

	std::vector<int32_t> textureIndices_;
	std::vector<TextureHelper*> images_;
	std::vector<Material> mats_;

	AnimatedGLTFObj(std::string gltfPath, DeviceHelper* deviceHelper, uint32_t globalVertexOffset, uint32_t globalIndexOffset);
	void loadGLTF(uint32_t globalVertexOffset, uint32_t globalIndexOffset);

	void createDescriptors();

	void drawIndexedOpaque(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
	void drawIndexedTransparent(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
	void drawIndexedOutline(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
	void renderShadow(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t cascadeIndex);
	void drawDepth(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, AnimSceneNode* node);
	void renderDepth(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);

	uint32_t getTotalVertices() { return this->totalVertices_; };
	uint32_t getTotalIndices() { return this->totalIndices_; };
	uint32_t getFirstVertex() { return this->globalFirstVertex; };
	uint32_t getFirstIndex() { return this->globalFirstIndex; };

	void recursiveRemove(AnimSceneNode* parentNode);
	void remove();
	void recursiveVertexAdd(std::vector<Vertex>* vertices, AnimSceneNode* parentNode);
	void recursiveIndexAdd(std::vector<uint32_t>* indices, AnimSceneNode* parentNode);
	void addVertices(std::vector<Vertex>* vertices);
	void addIndices(std::vector<uint32_t>* indices);

	std::vector<Animation> animations_;
	Animation walkAnim;
	Animation idleAnim;
	Animation runAnim;
	void updateAnimation(float deltaTime);

private:
	std::string gltfPath_;
	DeviceHelper* pDevHelper_;

	uint32_t globalFirstVertex;
	uint32_t globalFirstIndex;
	uint32_t totalIndices_;
	uint32_t totalVertices_;

	tinygltf::Model* pInputModel_;

	void loadImages();
	void loadTextures();
	void loadMaterials();
	void loadSkins();
	void loadAnimations(tinygltf::Model& in, std::vector<Animation>& animations);
	void updateJoints(AnimSceneNode* node);
	glm::mat4 getNodeMatrix(AnimSceneNode* node);
	void loadNode(tinygltf::Model& in, const tinygltf::Node& nodeIn, uint32_t index, AnimSceneNode* parent, std::vector<AnimSceneNode*>& nodes, uint32_t globalVertexOffset, uint32_t globalIndexOffset);
	void drawShadow(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t cascadeIndex, AnimSceneNode* node);
	AnimSceneNode* nodeFromIndex(uint32_t index);
	AnimSceneNode* findNode(AnimSceneNode* parent, uint32_t index);
	void callIndexedDraw(VkCommandBuffer& commandBuffer, MeshHelper::indirectDrawInfo& indexedDrawInfo);
};;
