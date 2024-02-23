#pragma once

#include <vector>
#include <glm/gtc/type_ptr.hpp>
#include <filesystem>
#include "MeshHelper.h"

class GLTFObj {
public:
	struct SceneNode {
		SceneNode* parent;
		std::vector<SceneNode*> children;
		MeshHelper::MeshObj mesh;
		glm::mat4 transform;
	};

	GLTFObj(std::string gltfPath, DeviceHelper* deviceHelper);
	void loadGLTF();

	void createDescriptors();
	void loadImages(tinygltf::Model& in, std::vector<TextureHelper*>& images);
	void loadTextures(tinygltf::Model& in, std::vector<TextureHelper::TextureIndexHolder>& textures);
	void loadMaterials(tinygltf::Model& in, std::vector<MeshHelper::Material>& mats);
	void loadNode(tinygltf::Model& in, const tinygltf::Node& nodeIn, SceneNode* parent, std::vector<SceneNode*>& nodes);

	void render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);

	uint32_t getTotalVertices() { return _totalVertices; };
	uint32_t getTotalIndices() { return _totalIndices; };
	MeshHelper* getMeshHelper() { return pSceneMesh; };
private:
	std::string _gltfPath;
	DeviceHelper* pDevHelper;
	uint32_t _totalIndices;
	uint32_t _totalVertices;
	MeshHelper* pSceneMesh;
	tinygltf::Model* pInputModel;
	std::vector<SceneNode*> pNodes;

	void drawIndexed(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, SceneNode* pNode);
};