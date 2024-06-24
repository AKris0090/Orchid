#pragma once

#include <vector>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>
#include "MeshHelper.h"
#include "mikktspace.h"

class GLTFObj {
public:
	bool transparentCurrentBound = false;

	struct SceneNode {
		SceneNode* parent;
		std::vector<SceneNode*> children;
		MeshHelper::MeshObj mesh;
		glm::mat4 transform;
	};

	struct depthMVModel {
		glm::mat4 model;
		uint32_t index;
	} depthPushBlock_;

	struct cascadeMVP {
		glm::mat4 model;
		uint32_t cascadeIndex;
	} cascadeBlock;

	struct drawOpaqueIndirectCall {
		uint32_t firstIndex;
		uint32_t numIndices;
	};

	struct pcBlock {
		int alphaMask;
		float alphaCutoff;
	} pushConstantBlock;

	struct drawTransparentIndirectCall {
		pcBlock loadedAlphaInfo;
		uint32_t firstIndex;
		uint32_t numIndices;
	};

	bool isSkyBox_ = false;
	MeshHelper* pSceneMesh_;
	std::vector<SceneNode*> pNodes_;
	glm::mat4 modelTransform;
	glm::vec3* pos;
	std::unordered_map<MeshHelper::Material*, std::vector<drawOpaqueIndirectCall*>> opaqueDraws;
	std::unordered_map<MeshHelper::Material*, std::vector<drawTransparentIndirectCall*>> transparentDraws;
	GLTFObj(std::string gltfPath, DeviceHelper* deviceHelper);
	void loadGLTF();

	void createDescriptors();

	struct UBO {
		glm::mat4 cascadeMVP[SHADOW_MAP_CASCADE_COUNT];
	} uniform;

	void render(VkCommandBuffer commandBuffer);
	void drawIndexedOpaque(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
	void drawIndexedTransparent(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
	void renderShadow(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t cascadeIndex, VkDescriptorSet cascadeDescriptor);
	void renderSkyBox(VkCommandBuffer commandBuffer, VkPipeline pipeline, VkDescriptorSet descSet, VkPipelineLayout pipelineLayout);
	void genImageRenderSkybox(VkCommandBuffer commandBuffer);

	uint32_t getTotalVertices() { return this->totalVertices_; };
	uint32_t getTotalIndices() { return this->totalIndices_; };
	MeshHelper* getMeshHelper() { return pSceneMesh_; };
private:
	std::string gltfPath_;
	DeviceHelper* pDevHelper_;
	uint32_t totalIndices_;
	uint32_t totalVertices_;
	tinygltf::Model* pInputModel_;

	void loadImages(tinygltf::Model& in, std::vector<TextureHelper*>& images);
	void loadTextures(tinygltf::Model& in, std::vector<TextureHelper::TextureIndexHolder>& textures);
	void loadMaterials(tinygltf::Model& in, std::vector<MeshHelper::Material>& mats);
	void loadNode(tinygltf::Model& in, const tinygltf::Node& nodeIn, SceneNode* parent, std::vector<SceneNode*>& nodes);
	void drawShadow(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t cascadeIndex, VkDescriptorSet cascadeDescriptor, SceneNode* node);
	void drawSkyBoxIndexed(VkCommandBuffer commandBuffer, VkPipeline pipeline, VkDescriptorSet descSet, VkPipelineLayout pipelineLayout, SceneNode* node);
	void genImageDrawSkyBoxIndexed(VkCommandBuffer commandBuffer, SceneNode* node);
};