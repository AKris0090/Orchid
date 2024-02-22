#pragma once

#include <iostream>
#include <vulkan/vulkan.hpp>

class DeviceHelper {
private:
    VkDevice _device;
    VkPhysicalDevice _gpu;
    VkCommandPool _commandPool;
    VkQueue _graphicsQueue;

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

public:
    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);

    VkDevice getDevice() { return this->_device; };
    VkPhysicalDevice getPhysicalDevice() { return this->_gpu; };
    VkCommandPool getCommandPool() { return this->_commandPool; };
    VkQueue getGraphicsQueue() { return this->_graphicsQueue; };
};