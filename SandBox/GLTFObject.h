#pragma once

#include "MeshHelper.h"

class GLTFObj {
public:
	struct SceneNode {
		SceneNode* parent;
		std::vector<SceneNode*> children;
		std::vector<MeshHelper*> meshPrimitives;
		glm::mat4 worldTransform;
	};

	uint32_t globalFirstVertex;
	uint32_t globalFirstIndex;
	uint32_t totalIndices_;
	uint32_t totalVertices_;
	glm::mat4 localModelTransform;
	bool isSkyBox_ = false;

	std::unordered_map<Material*, std::vector<MeshHelper*>> opaqueDraws;
	std::unordered_map<Material*, std::vector<MeshHelper*>> transparentDraws;

	std::vector<int32_t> textureIndices_;
	std::vector<Material> mats_;
	std::vector<Vertex> vertices_;
	std::vector<uint32_t> indices_;
	std::vector<TextureHelper*> images_;
	std::vector<SceneNode*> pParentNodes;

	void createDescriptors();

	GLTFObj(std::string gltfPath, DeviceHelper* deviceHelper, uint32_t globalVertexOffset, uint32_t globalIndexOffset);
	~GLTFObj();

private:
	std::string gltfPath_;
	DeviceHelper* pDevHelper_;
	tinygltf::Model* pInputModel_;

	void loadImages();
	void loadTextures();
	void loadMaterials();
	void loadNode(const tinygltf::Node& nodeIn, SceneNode* parent, uint32_t globalVertexOffset, uint32_t globalIndexOffset);
	void loadGLTF(uint32_t globalVertexOffset, uint32_t globalIndexOffset);

	void recursiveDeleteNode(SceneNode* node);
};