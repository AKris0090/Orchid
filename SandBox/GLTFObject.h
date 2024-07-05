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
		std::vector<MeshHelper*> meshPrimitives;
		glm::mat4 worldTransform;
	};

	std::vector<int32_t> textureIndices_;
	std::vector<TextureHelper*> images_;
	std::vector<Material> mats_;

	bool isSkyBox_ = false;

	std::vector<SceneNode*> pParentNodes;

	glm::mat4 modelTransform;
	glm::vec3* pos;
	std::unordered_map<Material*, std::vector<MeshHelper*>> opaqueDraws;
	std::unordered_map<Material*, std::vector<MeshHelper*>> transparentDraws;
	GLTFObj(std::string gltfPath, DeviceHelper* deviceHelper, uint32_t globalVertexOffset, uint32_t globalIndexOffset);
	void loadGLTF(uint32_t globalVertexOffset, uint32_t globalIndexOffset);

	std::vector<Vertex> objectVertices;
	std::vector<uint32_t> objectIndices;

	void createDescriptors();

	struct UBO {
		glm::mat4 cascadeMVP[SHADOW_MAP_CASCADE_COUNT];
	} uniform;

	void recursiveRemove(SceneNode* parentNode);
	void remove();
	void drawIndexedOpaque(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
	void drawIndexedTransparent(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
	void renderShadow(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t cascadeIndex);
	void drawShadow(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t cascadeIndex, SceneNode* node);
	void drawSkyBoxIndexed(VkCommandBuffer commandBuffer);
	void drawDepth(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, SceneNode* node);
	void renderDepth(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
	void addVertices(std::vector<Vertex>* vertices);
	void addIndices(std::vector<uint32_t>* indices);

	uint32_t getTotalVertices() { return this->totalVertices_; };
	uint32_t getTotalIndices() { return this->totalIndices_; };
	uint32_t getFirstVertex() { return this->globalFirstVertex; };
	uint32_t getFirstIndex() { return this->globalFirstIndex; };
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
	void loadNode(tinygltf::Model& in, const tinygltf::Node& nodeIn, SceneNode* parent, uint32_t globalVertexOffset, uint32_t globalIndexOffset);
	void callIndexedDraw(VkCommandBuffer& commandBuffer, MeshHelper::indirectDrawInfo& indexedDrawInfo);

	void recursiveVertexAdd(std::vector<Vertex>* vertices, SceneNode* node);
	void recursiveIndexAdd(std::vector<uint32_t>* indices, SceneNode* node);
};