#pragma once

#include <string>
#include <glm/glm.hpp>
#include <unordered_map>
#include <glm/gtx/hash.hpp>
#include "TextureHelper.h"

class MeshHelper {
public:
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
		VkPipeline pipeline;
	};

	// Structs from gltfloading example by Sascha Willems
	struct PrimitiveObjIndices {
		uint32_t firstIndex;
		uint32_t numIndices;
		int32_t materialIndex;
	};

	struct MeshObj {
		std::vector<PrimitiveObjIndices> primitives;
	};

	struct Vertex {
		glm::vec3 pos;
		glm::vec3 color;
		glm::vec2 uv;
		glm::vec3 normal;
		glm::vec4 tangent;

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions{};
			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, pos);
			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, color);
			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(Vertex, uv);
			attributeDescriptions[3].binding = 0;
			attributeDescriptions[3].location = 3;
			attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[3].offset = offsetof(Vertex, normal);
			attributeDescriptions[4].binding = 0;
			attributeDescriptions[4].location = 4;
			attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[4].offset = offsetof(Vertex, tangent);
			return attributeDescriptions;
		}

		bool operator==(const Vertex& other) const {
			return pos == other.pos && color == other.color && uv == other.uv && normal == other.normal;
		}
	};

	struct altVertex {
		glm::vec3 pos;

		static VkVertexInputBindingDescription getBindingDescription() {
			VkVertexInputBindingDescription bindingDescription{};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(altVertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 1> getAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions{};
			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(altVertex, pos);
			return attributeDescriptions;
		}

		bool operator==(const altVertex& other) const {
			return pos == other.pos;
		}
	};

	std::vector<TextureHelper*> images_;
	std::vector<TextureHelper::TextureIndexHolder> textures_;
	std::vector<Material> mats_;

	std::vector<uint32_t> indices_ = {};
	std::vector<MeshHelper::Vertex> vertices_ = {};

	MeshHelper(DeviceHelper* deviceHelper);

	void createVertexBuffer();
	void createIndexBuffer();

	VkBuffer getVertexBuffer() { return vertexBuffer_; };
	VkBuffer getIndexBuffer() { return indexBuffer_; };

private:
	VkDevice device_;
	DeviceHelper* pDevHelper_;

	VkBuffer vertexBuffer_;
	VkDeviceMemory vertexBufferMemory_;

	VkBuffer indexBuffer_;
	VkDeviceMemory indexBufferMemory_;
};

namespace std {
	template<> struct hash<MeshHelper::Vertex> {
		size_t operator()(MeshHelper::Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.uv) << 1);
		}
	};
}
