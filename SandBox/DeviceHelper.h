#pragma once

#include <iostream>
#include <vulkan/vulkan.hpp>
#include <fstream>
#include <vector>
#include <cstring>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/type_ptr.hpp>

#pragma warning(push, 0)

#include <PxPhysics.h>
#include <PxPhysicsAPI.h>
#include <PxCooking.h>
#include <characterkinematic/PxControllerManager.h>

#pragma warning(pop)

#include "SDL2/SDL.h"
#include "SDL2/SDL_vulkan.h"
#include <optional>
#include <set>
#include <algorithm>
#include <chrono>
#include <unordered_map>
#include <filesystem>

constexpr auto PI = 3.141592653589793;
constexpr auto SHADOW_MAP_CASCADE_COUNT = 4;


class DeviceHelper {
public:
    VkDevice device_;
    VkPhysicalDevice gpu_;
    VkCommandPool commandPool_;
    VkQueue graphicsQueue_;
    VkDescriptorPool descPool_;
    VkDescriptorSetLayout texDescSetLayout_;
    VkSampleCountFlagBits msaaSamples_;

    DeviceHelper() {
        this->device_ = VK_NULL_HANDLE;
        this->gpu_ = VK_NULL_HANDLE;
        this->commandPool_ = VK_NULL_HANDLE;
        this->graphicsQueue_ = VK_NULL_HANDLE;
        this->descPool_ = VK_NULL_HANDLE;
        this->texDescSetLayout_ = VK_NULL_HANDLE;
        this->msaaSamples_ = VK_SAMPLE_COUNT_1_BIT;
    };

    VkCommandBuffer beginSingleTimeCommands() const;
    void endSingleTimeCommands(VkCommandBuffer commandBuffer) const;
    void protectedEndCommands(VkCommandBuffer commandBuffer) const;

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const;
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const;
    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDeviceSize offset) const;

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

    void createImage(uint32_t width, uint32_t height, uint32_t mipLevel, uint16_t arrayLevels, VkImageCreateFlagBits flags, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) const;    void createImageView(const VkImage& image, VkImageView& imageView, const VkFormat format, const VkImageAspectFlags aspectFlags, const uint32_t mipLevels) const;
    void transitionImageLayout(VkCommandBuffer& cmdBuff, const VkImageSubresourceRange& subresourceRange, const VkImageLayout& oldLayout, const VkImageLayout& newLayout, VkImage& image);
};

static struct pcBlock {
    float alphaCutoff;
} pushConstantBlock;

class Transform {
public:
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 rotation = glm::vec3(0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    glm::vec3 forward = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 right = glm::vec3(0.0f, 0.0f, 1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::mat4 matrix = glm::mat4(1.0f);

    glm::mat4 to_matrix() {
        glm::mat4 retM = glm::translate(glm::mat4(1.0f), position);
        retM *= glm::mat4_cast(glm::quat(rotation));
        retM *= glm::scale(glm::mat4(1.0f), scale);
        return retM;
    }
};