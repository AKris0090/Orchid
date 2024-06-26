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

constexpr auto PI = 3.141592653589793;
constexpr auto SHADOW_MAP_CASCADE_COUNT = 4;


class DeviceHelper {
private:
    VkDevice device_;
    VkPhysicalDevice gpu_;
    VkCommandPool commandPool_;
    VkQueue graphicsQueue_;
    VkDescriptorPool descPool_;
    VkDescriptorSetLayout texDescSetLayout_;

public:
    DeviceHelper() {
        this->device_ = VK_NULL_HANDLE;
        this->gpu_ = VK_NULL_HANDLE;
        this->commandPool_ = VK_NULL_HANDLE;
        this->graphicsQueue_ = VK_NULL_HANDLE;
        this->descPool_ = VK_NULL_HANDLE;
        this->texDescSetLayout_ = VK_NULL_HANDLE;
    };

    VkCommandBuffer beginSingleTimeCommands() const;
    void endSingleTimeCommands(VkCommandBuffer commandBuffer) const;
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const;
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    void createImage(uint32_t width, uint32_t height, uint32_t mipLevel, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) const;
    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) const;

    void createSkyBoxImage(uint32_t width, uint32_t height, uint32_t mipLevel, uint16_t arrayLevels, VkImageCreateFlagBits flags, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) const;

    std::vector<char> readFile(const std::string& filename);

    VkDevice                    getDevice()                 const { return this->device_; };
    VkPhysicalDevice            getPhysicalDevice()         const { return this->gpu_; };
    VkCommandPool               getCommandPool()            const { return this->commandPool_; };
    VkQueue                     getGraphicsQueue()          const { return this->graphicsQueue_; };
    VkDescriptorPool            getDescriptorPool()         const { return this->descPool_; };
    const VkDescriptorSetLayout getTextureDescSetLayout()   const { return this->texDescSetLayout_; };

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

    glm::mat4 to_matrix() const {
        glm::mat4 retM = glm::translate(glm::mat4(1.0f), position);
        retM *= glm::mat4_cast(glm::quat(rotation));
        retM *= glm::scale(glm::mat4(1.0f), scale);
        return retM;
    }
};