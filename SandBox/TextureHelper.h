#pragma once

#include "DeviceHelper.h"
#include "stb_image.h"
#include <tiny_gltf.h>

class TextureHelper {
private:
    uint32_t mipLevels_;
    std::string texPath_;
    int index_;
    std::vector<std::string> texPaths_;
    VkImage textureImage_;
    VkDeviceMemory textureImageMemory_;
    DeviceHelper* pDevHelper_;
    tinygltf::Model* pInputModel_;

    void createTextureImages();
    void createTextureImageView(VkFormat f = VK_FORMAT_R8G8B8A8_SRGB);
    void createTextureImageSampler();

public:
    VkFormat imageFormat_ = VK_FORMAT_R8G8B8A8_UNORM;

    VkImageView textureImageView_;
    VkSampler textureSampler_;
    VkDescriptorSet descriptorSet_;

    static void generateMipmaps(VkCommandBuffer& commandBuffer, VkImage& image, DeviceHelper* pD, int arrayLayers, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
    static void copyBufferToImage(VkCommandBuffer& cmdBuff, VkBuffer& buffer, VkImage& image, VkImageLayout finalLayout, DeviceHelper* pD, int layerCount, uint32_t width, uint32_t height);
    void load();

    TextureHelper(tinygltf::Model& mod, int32_t textureIndex, DeviceHelper* pD);
    TextureHelper(std::string texPath, DeviceHelper* pD);
    ~TextureHelper();
};