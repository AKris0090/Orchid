#pragma once

#include <iostream>
#include <vulkan/vulkan.hpp>

class DeviceHelper {
private:
    VkDevice _device;
    VkPhysicalDevice _gpu;
    VkCommandPool _commandPool;
    VkQueue _graphicsQueue;
    VkDescriptorPool _descPool;
    VkDescriptorSetLayout _texDescSetLayout;

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

public:
    DeviceHelper();
    DeviceHelper(VkDevice device, VkPhysicalDevice gpu, VkCommandPool commandPool, VkQueue graphicsQueue, VkDescriptorPool descPool, VkDescriptorSetLayout texDescSetLayout);

    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

    void createImage(uint32_t width, uint32_t height, uint32_t mipLevel, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

    VkDevice getDevice() { return this->_device; };
    VkPhysicalDevice getPhysicalDevice() { return this->_gpu; };
    VkCommandPool getCommandPool() { return this->_commandPool; };
    VkQueue getGraphicsQueue() { return this->_graphicsQueue; };
    VkDescriptorPool getDescriptorPool() { return this->_descPool; };
    const VkDescriptorSetLayout getTextureDescSetLayout() { return this->_texDescSetLayout; };

    void setDevice(VkDevice dev) { this->_device = dev; };
    void setPhysicalDevice(VkPhysicalDevice pD) { this->_gpu = pD; };
    void setCommandPool(VkCommandPool comPool) { this->_commandPool = comPool; };
    void setGraphicsQueue(VkQueue gQ) { this->_graphicsQueue = gQ; };
    void setDescriptorPool(VkDescriptorPool desPool) { this->_descPool = desPool; };
    void setTextureDescSetLayout(VkDescriptorSetLayout descSet) { this->_texDescSetLayout = descSet; };
};