#pragma once

#include <iostream>
#include <vulkan/vulkan.hpp>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <PxPhysics.h>
#include <PxPhysicsAPI.h>
#include <PxCooking.h>
#include <characterkinematic/PxControllerManager.h>

#define PI 3.141592653589793
#define SHADOW_MAP_CASCADE_COUNT 4


class DeviceHelper {
private:
    VkDevice device_;
    VkPhysicalDevice gpu_;
    VkCommandPool commandPool_;
    VkQueue graphicsQueue_;
    VkDescriptorPool descPool_;
    VkDescriptorSetLayout texDescSetLayout_;

public:
    DeviceHelper() {};

    VkCommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(VkCommandBuffer commandBuffer);
    void endSingleTimeCommandsFenced(VkDevice device_, VkCommandBuffer cmdBuff);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    void createImage(uint32_t width, uint32_t height, uint32_t mipLevel, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    void createSkyBoxImage(uint32_t width, uint32_t height, uint32_t mipLevel, uint16_t arrayLevels, VkImageCreateFlagBits flags, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);

    std::vector<char> readFile(const std::string& filename);

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

class Transform {
public:
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    glm::vec3 forward = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 right = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::mat4 to_matrix() {
        glm::mat4 retM = glm::translate(glm::mat4(1.0f), position);
        retM *= glm::mat4_cast(glm::quat(rotation));
        retM *= glm::scale(glm::mat4(1.0f), scale);
        //std::cout << "TransformPosition: " << position.x << " , " << position.y << " , " << position.z << std::endl;
        return retM;
    }
};