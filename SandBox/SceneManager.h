#pragma once

#include <vector>
#include "MeshHelper.h"
#include "TextureHelper.h"

class Scene {
private:
	uint32_t totalIndices;

	uint32_t totalVertices;

	MeshHelper sceneMesh;

	struct SceneNode {
		SceneNode* parent;
		std::vector<SceneNode*> children;
		MeshHelper::MeshObj mesh;
		glm::mat4 transform;
	};
};