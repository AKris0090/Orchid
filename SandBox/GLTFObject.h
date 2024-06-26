#pragma once

#include <vector>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>
#include "MeshHelper.h"
#include "mikktspace.h"

class GLTFObj {
public:
	struct cascadeMVP {
		glm::mat4 model;
		uint32_t cascadeIndex;
	} cascadeBlock;

	struct pcBlock {
		int alphaMask;
		float alphaCutoff;
	};

	struct SceneNode {
		SceneNode* parent;
		std::vector<SceneNode*> children;
		MeshHelper* mesh;
		glm::mat4 worldTransform;
	};

	std::vector<int32_t> textureIndices_;
	std::vector<TextureHelper*> images_;
	std::vector<Material> mats_;

	bool isSkyBox_ = false;

	std::vector<SceneNode*> pParentNodes;

	glm::mat4 modelTransform;
	glm::vec3* pos;
	std::unordered_map<Material*, std::vector<SceneNode*>> opaqueDraws;
	std::unordered_map<Material*, std::vector<SceneNode*>> transparentDraws;
	GLTFObj(std::string gltfPath, DeviceHelper* deviceHelper);
	void loadGLTF(uint32_t globalVertexOffset, uint32_t globalIndexOffset);

	void createDescriptors();

	struct UBO {
		glm::mat4 cascadeMVP[SHADOW_MAP_CASCADE_COUNT];
	} uniform;

	void drawIndexedOpaque(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
	void drawIndexedTransparent(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
	void renderShadow(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t cascadeIndex, VkDescriptorSet cascadeDescriptor);
	void drawShadow(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t cascadeIndex, VkDescriptorSet cascadeDescriptor, SceneNode* node);
	void drawSkyBoxIndexed(VkCommandBuffer commandBuffer);

	uint32_t getTotalVertices() { return this->totalVertices_; };
	uint32_t getTotalIndices() { return this->totalIndices_; };
private:
	std::string gltfPath_;
	DeviceHelper* pDevHelper_;

	uint32_t totalIndices_;
	uint32_t totalVertices_;

	tinygltf::Model* pInputModel_;

	void loadImages();
	void loadTextures();
	void loadMaterials();
	void loadNode(tinygltf::Model& in, const tinygltf::Node& nodeIn, SceneNode* parent, uint32_t globalVertexOffset, uint32_t globalIndexOffset);
	void callIndexedDraw(VkCommandBuffer& commandBuffer, MeshHelper::indirectDrawInfo& indexedDrawInfo);
};