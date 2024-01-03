#pragma once

#ifndef MODELHELPER_H
#define MODELHELPER_H


#include <Vulkan/vulkan.h>
#include <cstdio>
#include <array>
#include "SDL2/SDL.h"
#include "SDL2/SDL_vulkan.h"
#include <vector>
#include <cstring>
#include <optional>
#include <set>
#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <unordered_map>
#include <chrono>

#include <glm/gtx/hash.hpp>

class VulkanRenderer;

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 texCoord;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
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
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
		return attributeDescriptions;
	}

	bool operator==(const Vertex& other) const {
		return pos == other.pos && color == other.color && texCoord == other.texCoord;
	}
};

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
		}
	};
}

struct Model {
	uint32_t totalIndices;
	uint32_t totalVertices;
	glm::mat4 transform{ 1.0 };

	std::vector<uint32_t> indices = {};
	std::vector<Vertex> vertices = {};
};

class ModelHelper {
private:
	std::string modPath;
	VulkanRenderer* vkR;
	glm::mat4 transform;

	// Vertex Buffer Handle
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	// Index buffer handle
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	void createVertexBuffer();
	void createIndexBuffer();

public:
	Model model;
	int textureIndex = 0;
	ModelHelper(std::string path, VulkanRenderer* vkR);

	void load();
	void destroy();
	void render(VkCommandBuffer commandBuffer);
};

#endif