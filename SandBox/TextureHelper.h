#pragma once

#include <iostream>
#include <vulkan/vulkan.hpp>
#include "DeviceHelper.h"

#include <tiny_gltf.h>

class TextureHelper {
private:
    tinygltf::Model* pInputModel_;
    uint32_t mipLevels_;
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
    VkFormat imageFormat_ = VK_FORMAT_R8G8B8A8_UNORM;
    VkImageView textureImageView_;
    VkSampler textureSampler_;

    void createTextureImageView(VkFormat f = VK_FORMAT_R8G8B8A8_SRGB);
    void createTextureImageSampler();

    VkDescriptorSet getDescriptorSet() const { return this->descriptorSet_; };

    void load();
    void loadSkyBoxTex();

    TextureHelper(tinygltf::Model& mod, int32_t textureIndex, DeviceHelper* pD) {
        this->pDevHelper_ = pD;
        this->device_ = pD->getDevice();
        this->pInputModel_ = &mod;
        this->mipLevels_ = VK_SAMPLE_COUNT_1_BIT;
        this->texPath_ = "";
        this->index_ = textureIndex;
        this->textureImage_ = VK_NULL_HANDLE;
        this->textureImageMemory_ = VK_NULL_HANDLE;
        this->descriptorSet_ = VK_NULL_HANDLE;
    };

    TextureHelper(std::string texPath, DeviceHelper* pD) {
        this->pDevHelper_ = pD;
        this->device_ = pD->getDevice();
        this->pInputModel_ = nullptr;
        this->mipLevels_ = VK_SAMPLE_COUNT_1_BIT;
        this->texPath_ = texPath;
        this->index_ = INT_MIN;
        this->textureImage_ = VK_NULL_HANDLE;
        this->textureImageMemory_ = VK_NULL_HANDLE;
        this->descriptorSet_ = VK_NULL_HANDLE;
    };
};