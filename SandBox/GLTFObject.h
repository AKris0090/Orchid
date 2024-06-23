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

	struct pcBlock {
		int alphaMask;
		float alphaCutoff;
	} pushConstantBlock;

	bool isSkyBox_ = false;
	MeshHelper* pSceneMesh_;

	glm::mat4 modelTransform;
	glm::vec3* pos;

	GLTFObj(std::string gltfPath, DeviceHelper* deviceHelper);
	void loadGLTF();

	void createDescriptors();

	struct UBO {
		glm::mat4 cascadeMVP[SHADOW_MAP_CASCADE_COUNT];
	} uniform;

	void render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VkPipeline opaquePipeline, VkPipeline transparentPipeline);
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
	std::vector<SceneNode*> pNodes_;

	void loadImages(tinygltf::Model& in, std::vector<TextureHelper*>& images);
	void loadTextures(tinygltf::Model& in, std::vector<TextureHelper::TextureIndexHolder>& textures);
	void loadMaterials(tinygltf::Model& in, std::vector<MeshHelper::Material>& mats);
	void loadNode(tinygltf::Model& in, const tinygltf::Node& nodeIn, SceneNode* parent, std::vector<SceneNode*>& nodes);
	void drawIndexed(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, SceneNode* pNode, VkPipeline opaquePipeline, VkPipeline transparentPipeline);
	void drawShadow(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t cascadeIndex, VkDescriptorSet cascadeDescriptor, SceneNode* node);
	void drawSkyBoxIndexed(VkCommandBuffer commandBuffer, VkPipeline pipeline, VkDescriptorSet descSet, VkPipelineLayout pipelineLayout, SceneNode* node);
	void genImageDrawSkyBoxIndexed(VkCommandBuffer commandBuffer, SceneNode* node);
};