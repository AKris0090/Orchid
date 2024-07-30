#pragma once

#include <string>
#include <glm/glm.hpp>
#include <unordered_map>
#include <glm/gtx/hash.hpp>
#include "TextureHelper.h"

struct Material {
	glm::vec4 baseColor = glm::vec4(1.0f);
	uint32_t baseColorTexIndex;
	uint32_t normalTexIndex;
	uint32_t metallicRoughnessIndex;
	uint32_t aoIndex;
	uint32_t emissionIndex;
	std::string alphaMode = "OPAQUE";
	float alphaCutOff;
	bool doubleSides = false;
	VkDescriptorSet descriptorSet;
};

struct Vertex {
	glm::vec4 pos; // uv_x stored in w
	glm::vec4 normal; // uv_y stored in w
	glm::vec4 tangent;
	glm::vec4 jointIndices;
	glm::vec4 jointWeights;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 1> getPositionAttributeDescription() {
		std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions{};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);
		return attributeDescriptions;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, normal);
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, tangent);
		return attributeDescriptions;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getDepthAttributeDescription() {
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, normal);
		return attributeDescriptions;
	}

	static std::array<VkVertexInputAttributeDescription, 5> getAnimatedAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions{};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, normal);
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, tangent);
		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(Vertex, jointIndices);
		attributeDescriptions[4].binding = 0;
		attributeDescriptions[4].location = 4;
		attributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[4].offset = offsetof(Vertex, jointWeights);
		return attributeDescriptions;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAnimatedPositionDescription() {
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, jointIndices);
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, jointWeights);
		return attributeDescriptions;
	}

	bool operator==(const Vertex& other) const {
		return pos == other.pos && normal == other.normal && tangent == other.tangent;
	}

	Vertex(glm::vec2 pos, glm::vec2 tex) {
		this->pos.x = pos.x;
		this->pos.y = pos.y;
		this->pos.z = tex.x;
		this->pos.w = tex.y;
	}

	Vertex() {};
};

class MeshHelper {
public:
	uint32_t materialIndex;
	uint32_t skinIndex;	
	glm::mat4* worldTransformMatrix;
	std::vector<uint32_t> stagingIndices_ = {};
	std::vector<Vertex> stagingVertices_ = {};

	MeshHelper() {};

	struct indirectDrawInfo {
		uint32_t firstIndex;
		uint32_t numIndices;
		uint32_t firstInstance;
		uint32_t instanceCount;
		uint32_t globalVertexOffset;
		uint32_t numVertices;
	} indirectInfo;
};

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return (hash<glm::vec4>()(vertex.pos) ^ hash<glm::vec4>()(vertex.normal) ^ hash<glm::vec4>()(vertex.tangent));
		}
	};
}
