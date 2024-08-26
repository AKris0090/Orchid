#include "DeviceHelper.h"

void DeviceHelper::createImageView(const VkImage& image, VkImageView& imageView, const VkFormat format, const VkImageAspectFlags aspectFlags, const uint32_t mipLevels) const {
    VkImageViewCreateInfo imageViewCInfo{};
    imageViewCInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCInfo.image = image;
    imageViewCInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCInfo.format = format;
    imageViewCInfo.subresourceRange.aspectMask = aspectFlags;
    imageViewCInfo.subresourceRange.baseMipLevel = 0;
    imageViewCInfo.subresourceRange.levelCount = mipLevels;
    imageViewCInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCInfo.subresourceRange.layerCount = 1;

    if (vkCreateImageView(device_, &imageViewCInfo, nullptr, &imageView) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create a texture image view!");
    }
}

void DeviceHelper::createImage(uint32_t width, uint32_t height, uint32_t mipLevel, uint16_t arrayLevels, VkImageCreateFlagBits flags, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) const {
    VkImageCreateInfo imageCInfo{};
    imageCInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCInfo.extent.width = width;
    imageCInfo.extent.height = height;
    imageCInfo.extent.depth = 1;
    imageCInfo.mipLevels = mipLevel;
    imageCInfo.arrayLayers = arrayLevels;
    imageCInfo.format = format;
    imageCInfo.tiling = tiling;
    imageCInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCInfo.usage = usage;
    imageCInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCInfo.samples = numSamples;
    imageCInfo.flags = flags;

    if (vkCreateImage(device_, &imageCInfo, nullptr, &image) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create an image!");
    }

    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(device_, image, &memoryRequirements);

    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = memoryRequirements.size;
    allocateInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device_, &allocateInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        std::cout << "couldn't allocate lol" << std::endl;
        std::_Xruntime_error("Failed to allocate the image memory!");
    }

    vkBindImageMemory(device_, image, imageMemory, 0);
}

uint32_t DeviceHelper::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(gpu_, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    std::_Xruntime_error("Failed to find a suitable memory type!");
}

VkCommandBuffer DeviceHelper::beginSingleTimeCommands() const {
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandPool = commandPool_;
    allocateInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device_, &allocateInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void DeviceHelper::endSingleTimeCommands(VkCommandBuffer commandBuffer) const {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue_);

    vkFreeCommandBuffers(device_, commandPool_, 1, &commandBuffer);
}

void DeviceHelper::protectedEndCommands(VkCommandBuffer commandBuffer) const {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo queueSubmitInfo{};
    queueSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    queueSubmitInfo.commandBufferCount = 1;
    queueSubmitInfo.pCommandBuffers = &commandBuffer;
    // Create fence to ensure that the command buffer has finished executing
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence fence;
    vkCreateFence(device_, &fenceInfo, nullptr, &fence);
    // Submit to the queue
    vkQueueSubmit(graphicsQueue_, 1, &queueSubmitInfo, fence);
    // Wait for the fence to signal that command buffer has finished executing
    vkWaitForFences(device_, 1, &fence, VK_TRUE, 100000000000); // big number is fence timeout
    vkDestroyFence(device_, fence, nullptr);

    vkFreeCommandBuffers(device_, commandPool_, 1, &commandBuffer);
}

void DeviceHelper::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) const {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}

void DeviceHelper::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const {
    VkBufferCreateInfo bufferCInfo{};
    bufferCInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCInfo.size = size;
    bufferCInfo.usage = usage;
    bufferCInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device_, &bufferCInfo, nullptr, &buffer) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create the vertex buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device_, buffer, &memRequirements);

    VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{};
    memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    memoryAllocateFlagsInfo.flags = 0;

    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.pNext = &memoryAllocateFlagsInfo;
    allocateInfo.allocationSize = memRequirements.size;
    allocateInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device_, &allocateInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device_, buffer, bufferMemory, 0);
}

void DeviceHelper::transitionImageLayout(VkCommandBuffer& cmdBuff, const VkImageSubresourceRange& subresourceRange, const VkImageLayout& oldLayout, const VkImageLayout& newLayout, VkImage& image) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.image = image;
    barrier.subresourceRange = subresourceRange;

    switch (oldLayout) {
    case VK_IMAGE_LAYOUT_UNDEFINED:
        barrier.srcAccessMask = 0;
        break;
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;
    default:
        break;
    }

    switch (newLayout) {
    case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        break;
    case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        break;
    case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
        if (barrier.srcAccessMask == 0)
        {
            barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        }
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        break;
    }

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    vkCmdPipelineBarrier(cmdBuff, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
}