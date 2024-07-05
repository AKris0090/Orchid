#include "TextureHelper.h"

#define STBI_MSC_SECURE_CRT
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>

VkImageView createImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
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

    VkImageView tempImageView;
    if (vkCreateImageView(device, &imageViewCInfo, nullptr, &tempImageView) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create a texture image view!");
    }

    return tempImageView;
}

void TextureHelper::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
    VkCommandBuffer commandBuffer = pDevHelper_->beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    pDevHelper_->endSingleTimeCommands(commandBuffer);
}

void TextureHelper::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = pDevHelper_->beginSingleTimeCommands();
    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width = width;
    region.imageExtent.height = height;
    region.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    VkImageMemoryBarrier imageMemoryBarrier{};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageMemoryBarrier.subresourceRange.levelCount = 1;
    imageMemoryBarrier.subresourceRange.layerCount = 1;

    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

    pDevHelper_->endSingleTimeCommands(commandBuffer);
}

void TextureHelper::createTextureImages() {
    unsigned char* buff = nullptr;
    VkDeviceSize buffSize = 0;
    bool deleteBuff = false;
    int texWidth, texHeight, texChannels;
    if (pInputModel_->images.size() == 0) {
        return;
    }
    tinygltf::Image& curImage = pInputModel_->images[0];
    VkDeviceSize imageSize;
    bool dummy = false;
    stbi_uc* pixels = nullptr;
    switch (index_) {
    case -1:
        pixels = stbi_load("C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/dummyAO.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        imageSize = texWidth * texHeight * 4;
        this->mipLevels_ = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        dummy = true;
        break;
    case -2:
        pixels = stbi_load("C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/dummyMetallicRoughness.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        imageSize = texWidth * texHeight * 4;
        this->mipLevels_ = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        dummy = true;
        break;
    case -3:
        pixels = stbi_load("C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/dummyNormal.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        imageSize = texWidth * texHeight * 4;
        this->mipLevels_ = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        dummy = true;
        break;
    case -4:
        pixels = stbi_load("C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/dummyEmission.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        imageSize = texWidth * texHeight * 4;
        this->mipLevels_ = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        dummy = true;
        break;
    default:
        curImage = pInputModel_->images[index_];

        // load images
        if (curImage.component == 3) {
            buffSize = curImage.width * curImage.height * 4;
            buff = new unsigned char[buffSize];
            unsigned char* rgba = buff;
            unsigned char* rgb = &curImage.image[0];
            for (size_t j = 0; j < curImage.width * curImage.height; j++) {
                memcpy(rgba, rgb, sizeof(unsigned char) * 3);
                rgba += 4;
                rgb += 3;
            }
            deleteBuff = true;
        }
        else {
            buff = &curImage.image[0];
            buffSize = curImage.image.size();
        }

        this->mipLevels_ = static_cast<uint32_t>(std::floor(std::log2(std::max(curImage.width, curImage.height)))) + 1;
    }

    std::cout << "MIP LEVELS: " << this->mipLevels_ << std::endl;

    if (dummy) {
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        pDevHelper_->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device_, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(pDevHelper_->getDevice(), stagingBufferMemory);

        stbi_image_free(pixels);

        pDevHelper_->createImage(texWidth, texHeight, mipLevels_, VK_SAMPLE_COUNT_1_BIT, imageFormat_, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage_, textureImageMemory_);
        transitionImageLayout(textureImage_, imageFormat_, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels_);
        copyBufferToImage(stagingBuffer, textureImage_, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

        vkDestroyBuffer(pDevHelper_->getDevice(), stagingBuffer, nullptr);
        vkFreeMemory(pDevHelper_->getDevice(), stagingBufferMemory, nullptr);

        generateMipmaps(textureImage_, imageFormat_, curImage.width, curImage.height, this->mipLevels_);

        std::cout << "loaded: DUMMY " << index_ << std::endl;
    }
    else {
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        pDevHelper_->createBuffer(buffSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        // could mess up since buffer no longer requires same memory // DELETE IF WORKING
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device_, stagingBuffer, &memRequirements);

        void* data;
        vkMapMemory(device_, stagingBufferMemory, 0, memRequirements.size, 0, &data);
        memcpy(data, buff, buffSize);
        vkUnmapMemory(device_, stagingBufferMemory);

        pDevHelper_->createImage(curImage.width, curImage.height, mipLevels_, VK_SAMPLE_COUNT_1_BIT, imageFormat_, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage_, textureImageMemory_);
        transitionImageLayout(textureImage_, imageFormat_, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels_);

        copyBufferToImage(stagingBuffer, textureImage_, curImage.width, curImage.height);

        vkDestroyBuffer(device_, stagingBuffer, nullptr);
        vkFreeMemory(device_, stagingBufferMemory, nullptr);

        generateMipmaps(textureImage_, imageFormat_, curImage.width, curImage.height, this->mipLevels_);

        if (deleteBuff) {
            delete[] buff;
        }

        std::cout << "loaded: " << curImage.uri << std::endl;
    }
}


void TextureHelper::generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
    // Check if image format supports linear blitting
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(pDevHelper_->getPhysicalDevice(), imageFormat, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    VkCommandBuffer commandBuffer = pDevHelper_->beginSingleTimeCommands();

    for (uint32_t i = 1; i < mipLevels; i++) {
        VkImageBlit blit{};

        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.layerCount = 1;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcOffsets[1].x = int32_t(texWidth >> (i - 1));
        blit.srcOffsets[1].y = int32_t(texHeight >> (i - 1));
        blit.srcOffsets[1].z = 1;

        // Destination
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.layerCount = 1;
        blit.dstSubresource.mipLevel = i;
        blit.dstOffsets[1].x = int32_t(texWidth >> i);
        blit.dstOffsets[1].y = int32_t(texHeight >> i);
        blit.dstOffsets[1].z = 1;

        VkImageSubresourceRange mipSubRange = {};
        mipSubRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        mipSubRange.baseMipLevel = i;
        mipSubRange.levelCount = 1;
        mipSubRange.layerCount = 1;

        VkImageMemoryBarrier imageMemoryBarrier{};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.srcAccessMask = 0;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageMemoryBarrier.image = image;
        imageMemoryBarrier.subresourceRange = mipSubRange;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);

        vkCmdBlitImage(commandBuffer,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &imageMemoryBarrier);
    }

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.levelCount = this->mipLevels_;
    subresourceRange.layerCount = 1;

    VkImageMemoryBarrier imageMemoryBarrier{};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange = subresourceRange;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &imageMemoryBarrier);

    pDevHelper_->endSingleTimeCommands(commandBuffer);
}

void TextureHelper::createTextureImageView(VkFormat f) {
    textureImageView_ = pDevHelper_->createImageView(textureImage_, imageFormat_, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels_);
}

void TextureHelper::createTextureImageSampler() {
    VkSamplerCreateInfo samplerCInfo{};
    samplerCInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCInfo.magFilter = VK_FILTER_LINEAR;
    samplerCInfo.minFilter = VK_FILTER_LINEAR;
    samplerCInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCInfo.mipLodBias = 0.0f;
    samplerCInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerCInfo.minLod = 0.0f;
    samplerCInfo.maxLod = (float) this->mipLevels_;
    samplerCInfo.anisotropyEnable = VK_TRUE;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(pDevHelper_->getPhysicalDevice(), &properties);

    samplerCInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerCInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerCInfo.mipLodBias = 0.0f;

    if (vkCreateSampler(pDevHelper_->getDevice(), &samplerCInfo, nullptr, &textureSampler_) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create the texture sampler!");
    }
}

void TextureHelper::load() {
    createTextureImages();
    createTextureImageView();
    createTextureImageSampler();
}