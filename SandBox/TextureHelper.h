#pragma once

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

class TextureHelper {
private:
	uint32_t mipLevels;

	// Texture image and mem handles
	VkImage textureImage;
	VkDeviceMemory textureImageMemory;
	VkImageView textureImageView;
	VkSampler textureSampler;

	// Helpers
	std::string texPath;
	VulkanRenderer* vkR;

	void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
	void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

	void createTextureImage(const char* path);
	void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
	void createTextureImageView();
	void createTextureImageSampler();

	void createTextureDescriptorSet();

public:
	VkDescriptorSet descriptorSet;

	TextureHelper(std::string path, VulkanRenderer* vkR);

	void load();
	void free();
	void destroy();
};