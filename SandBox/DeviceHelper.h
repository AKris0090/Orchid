#pragma once

#include <iostream>
#include <vulkan/vulkan.hpp>

#define PI 3.141592653589793

class DeviceHelper {
private:
    VkDevice device_;
    VkPhysicalDevice gpu_;
    VkCommandPool commandPool_;
    VkQueue graphicsQueue_;
    VkDescriptorPool descPool_;
    VkDescriptorSetLayout texDescSetLayout_;

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

public:
    DeviceHelper() {};

    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    void createImage(uint32_t width, uint32_t height, uint32_t mipLevel, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    void createSkyBoxImage(uint32_t width, uint32_t height, uint32_t mipLevel, uint16_t arrayLevels, VkImageCreateFlagBits flags, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

    VkDevice                    getDevice()                 { return this->device_; };
    VkPhysicalDevice            getPhysicalDevice()         { return this->gpu_; };
    VkCommandPool               getCommandPool()            { return this->commandPool_; };
    VkQueue                     getGraphicsQueue()          { return this->graphicsQueue_; };
    VkDescriptorPool            getDescriptorPool()         { return this->descPool_; };
    const VkDescriptorSetLayout getTextureDescSetLayout()   { return this->texDescSetLayout_; };

    void setDevice(VkDevice dev)                                { this->device_ = dev; };
    void setPhysicalDevice(VkPhysicalDevice pD)                 { this->gpu_ = pD; };
    void setCommandPool(VkCommandPool comPool)                  { this->commandPool_ = comPool; };
    void setGraphicsQueue(VkQueue gQ)                           { this->graphicsQueue_ = gQ; };
    void setDescriptorPool(VkDescriptorPool desPool)            { this->descPool_ = desPool; };
    void setTextureDescSetLayout(VkDescriptorSetLayout descSet) { this->texDescSetLayout_ = descSet; };
};