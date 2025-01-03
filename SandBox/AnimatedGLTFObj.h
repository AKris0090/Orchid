#pragma once

#include "Animation.h"

class AnimatedGLTFObj {
public:
	struct Skin {
		std::string name;
		AnimSceneNode* skeletonRoot = nullptr;
		std::vector<glm::mat4> inverseBindMatrices;
		std::vector<AnimSceneNode*> joints;
		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
		std::vector<glm::mat4>* finalJointMatrices = nullptr;
	};

	struct secondaryTransform {
		glm::vec3 position{ 0.0f };
		glm::quat rotation{ 1.0f, 0.0f, 0.0f, 0.0f };
		glm::vec3 scale{ 1.0f };
	};

	uint32_t globalSkinningMatrixOffset;
	uint32_t globalFirstVertex;
	uint32_t globalFirstIndex;
	uint32_t totalIndices_;
	uint32_t totalVertices_;
	int numInverseBindMatrices;

	std::vector<secondaryTransform>* src;
	std::vector<secondaryTransform>* dst;
	Animation baseAnim;

	std::unordered_map<Material*, std::vector<MeshHelper*>> opaqueDraws;
	std::unordered_map<Material*, std::vector<MeshHelper*>> transparentDraws;

	std::vector<Skin> skins_;

	std::vector<int32_t> textureIndices_;
	std::vector<TextureHelper*> images_;
	std::vector<Material> mats_;
	std::vector<Vertex> basePoseVertices_;
	std::vector<uint32_t> indices_;
	std::vector<AnimSceneNode*> pParentNodes;

	VkBuffer vertexBuffer_;
	VkDeviceMemory vertexBufferMemory_;

	void createDescriptors();

	AnimatedGLTFObj() { this->numInverseBindMatrices = 0; };
	void setup(std::string gltfPath, DeviceHelper* deviceHelper, uint32_t globalVertexOffset, uint32_t globalIndexOffset);
	~AnimatedGLTFObj();

private:
	std::string gltfPath_;
	DeviceHelper* pDevHelper_;
	tinygltf::Model* pInputModel_;

	void loadImages();
	void loadTextures();
	void loadMaterials();
	void loadSkins();
	void updateJoints(AnimSceneNode* node);
	glm::mat4 getNodeMatrix(AnimSceneNode* node);
	AnimSceneNode* nodeFromIndex(uint32_t index);
	AnimSceneNode* findNode(AnimSceneNode* parent, uint32_t index);
	void loadNode(tinygltf::Model& in, const tinygltf::Node& nodeIn, uint32_t index, AnimSceneNode* parent, std::vector<AnimSceneNode*>& nodes, uint32_t globalVertexOffset, uint32_t globalIndexOffset);
	void loadGLTF(uint32_t globalVertexOffset, uint32_t globalIndexOffset);
	void recursiveDeleteNode(AnimSceneNode* node);
};;
