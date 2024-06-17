#pragma once

#include <iostream>
#include <vulkan/vulkan.hpp>
#include "DeviceHelper.h"

#include <tiny_gltf.h>

class TextureHelper {
private:
    tinygltf::Model* pInputModel_;
    uint32_t mipLevels_ = VK_SAMPLE_COUNT_1_BIT;
    std::string texPath_; 
    VkDevice device_;
    DeviceHelper* pDevHelper_;
    int index_;
    VkImage textureImage_;
    VkDeviceMemory textureImageMemory_;
    VkDescriptorSet descriptorSet_;

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    void createTextureImages();
    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

public:
    struct TextureIndexHolder {
        uint32_t textureIndex;
    };

    VkFormat imageFormat_ = VK_FORMAT_R8G8B8A8_UNORM;
    VkImageView textureImageView_;
    VkSampler textureSampler_;

    TextureHelper() {};
    TextureHelper(tinygltf::Model& in, int i, DeviceHelper* deviceHelper);

    TextureHelper(std::string texPath, DeviceHelper* deviceHelper);

    void createTextureImageView(VkFormat f = VK_FORMAT_R8G8B8A8_SRGB);
    void createTextureImageSampler();

    VkDescriptorSet getDescriptorSet() { return this->descriptorSet_; };

    void load();
    void loadSkyBoxTex();
};