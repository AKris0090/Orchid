#pragma once

#include <tiny_gltf.h>
#include <iostream>
#include <vulkan/vulkan.hpp>
#include "DeviceHelper.h"

class TextureHelper {
private:
    DeviceHelper deviceHelper;
    tinygltf::Model* inputModel;

    uint32_t mipLevels = VK_SAMPLE_COUNT_1_BIT;

    // Helpers
    std::string texPath;

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    void createTextureImages();
    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

public:
    struct TextureIndexHolder {
        uint32_t textureIndex;
    };

    VkDevice _device;

    // Texture image and mem handles
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;

    VkSampler textureSampler;
    VkDescriptorSet descriptorSet;
    VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
    int i;
    TextureHelper(tinygltf::Model& in, int i);

    void createTextureImageView(VkFormat f = VK_FORMAT_R8G8B8A8_SRGB);
    void createTextureImageSampler();

    void load();
    void free();
    void destroy();
};