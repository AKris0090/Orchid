#pragma once

#include <string>
#include <glm/glm.hpp>
#include <unordered_map>
#include "TextureHelper.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

class MeshHelper {
public:
	// A glTF material stores information in e.g. the texture that is attached to it and colors
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

	std::vector<TextureHelper*> images;
	std::vector<TextureHelper::TextureIndexHolder> textures;
	std::vector<Material> mats;

	MeshHelper();
	MeshHelper(VkDevice dev, VkPhysicalDevice GPU, VkQueue graphicsQueue, VkCommandPool commandPool, std::string path);

	void loadGLTF();
	void createDescriptors();
	void destroy();
	void render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);

private:
	VkDevice _device;
	VkPhysicalDevice _gpu;
	VkQueue _graphicsQueue;
	VkCommandPool _commandPool;
	std::vector<uint32_t> indices = {};
	std::vector<MeshHelper::Vertex> vertices = {};

	std::string modelPath;

	// Vertex Buffer Handle
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	// Index buffer handle
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	void createVertexBuffer();
	void createIndexBuffer();
};

namespace std {
	template<> struct hash<MeshHelper::Vertex> {
		size_t operator()(MeshHelper::Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.uv) << 1);
		}
	};
}
