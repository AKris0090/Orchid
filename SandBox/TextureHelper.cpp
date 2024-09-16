#include "TextureHelper.h"

#define STBI_MSC_SECURE_CRT
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>

void TextureHelper::copyBufferToImage(VkCommandBuffer& cmdBuff, VkBuffer& buffer, VkImage& image, VkImageLayout finalLayout, DeviceHelper* pD, int layerCount, uint32_t width, uint32_t height) {
    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = layerCount;
    region.imageExtent.width = width;
    region.imageExtent.height = height;
    region.imageExtent.depth = 1;

    vkCmdCopyBufferToImage(cmdBuff, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    VkImageMemoryBarrier imageMemoryBarrier{};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageMemoryBarrier.newLayout = finalLayout;
    imageMemoryBarrier.image = image;
    imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageMemoryBarrier.subresourceRange.levelCount = 1;
    imageMemoryBarrier.subresourceRange.layerCount = layerCount;

    vkCmdPipelineBarrier(cmdBuff, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
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
        imageSize = static_cast<VkDeviceSize>(texWidth) * texHeight * 4;
        this->mipLevels_ = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        dummy = true;
        break;
    case -2:
        pixels = stbi_load("C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/dummyMetallicRoughness.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        imageSize = static_cast<VkDeviceSize>(texWidth) * texHeight * 4;
        this->mipLevels_ = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        dummy = true;
        break;
    case -3:
        pixels = stbi_load("C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/dummyNormal.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        imageSize = static_cast<VkDeviceSize>(texWidth) * texHeight * 4;
        this->mipLevels_ = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        dummy = true;
        break;
    case -4:
        pixels = stbi_load("C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/dummyEmission.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        imageSize = static_cast<VkDeviceSize>(texWidth) * texHeight * 4;
        this->mipLevels_ = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        dummy = true;
        break;
    default:
        curImage = pInputModel_->images[index_];

        if (curImage.component == 3) {
            buffSize = static_cast<VkDeviceSize>(curImage.width) * curImage.height * 4;
            buff = new unsigned char[buffSize];
            unsigned char* rgba = buff;
            unsigned char* rgb = &curImage.image[0];
            for (size_t j = 0; j < static_cast<unsigned long long>(curImage.width) * curImage.height; j++) {
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

    if (dummy) {
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        pDevHelper_->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(pDevHelper_->device_, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(pDevHelper_->device_, stagingBufferMemory);

        stbi_image_free(pixels);

        VkImageSubresourceRange subresource{};
        subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresource.levelCount = mipLevels_;
        subresource.layerCount = 1;

        pDevHelper_->createImage(texWidth, texHeight, mipLevels_, 1, static_cast<VkImageCreateFlagBits>(0), VK_SAMPLE_COUNT_1_BIT, imageFormat_, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage_, textureImageMemory_);
        
        VkCommandBuffer cmdBuff = pDevHelper_->beginSingleTimeCommands();
        pDevHelper_->transitionImageLayout(cmdBuff, subresource, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, textureImage_);
        copyBufferToImage(cmdBuff, stagingBuffer, textureImage_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, pDevHelper_, 1, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        generateMipmaps(cmdBuff, textureImage_, pDevHelper_, 1, imageFormat_, curImage.width, curImage.height, this->mipLevels_);
        pDevHelper_->endSingleTimeCommands(cmdBuff);

        vkDestroyBuffer(pDevHelper_->device_, stagingBuffer, nullptr);
        vkFreeMemory(pDevHelper_->device_, stagingBufferMemory, nullptr);

        std::cout << "loaded: DUMMY " << index_ << std::endl;
    }
    else {
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        pDevHelper_->createBuffer(buffSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(pDevHelper_->device_, stagingBuffer, &memRequirements);

        void* data;
        vkMapMemory(pDevHelper_->device_, stagingBufferMemory, 0, memRequirements.size, 0, &data);
        memcpy(data, buff, buffSize);
        vkUnmapMemory(pDevHelper_->device_, stagingBufferMemory);

        VkImageSubresourceRange subresource{};
        subresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subresource.levelCount = mipLevels_;
        subresource.layerCount = 1;

        pDevHelper_->createImage(curImage.width, curImage.height, mipLevels_, 1, static_cast<VkImageCreateFlagBits>(0), VK_SAMPLE_COUNT_1_BIT, imageFormat_, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage_, textureImageMemory_);
        
        VkCommandBuffer cmdBuff = pDevHelper_->beginSingleTimeCommands();

        pDevHelper_->transitionImageLayout(cmdBuff, subresource, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, textureImage_);
        copyBufferToImage(cmdBuff, stagingBuffer, textureImage_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, pDevHelper_, 1, curImage.width, curImage.height);

        generateMipmaps(cmdBuff, textureImage_, pDevHelper_, 1, imageFormat_, curImage.width, curImage.height, this->mipLevels_);

        pDevHelper_->endSingleTimeCommands(cmdBuff);

        vkDestroyBuffer(pDevHelper_->device_, stagingBuffer, nullptr);
        vkFreeMemory(pDevHelper_->device_, stagingBufferMemory, nullptr);

        if (deleteBuff) {
            delete[] buff;
        }

        if (curImage.uri != "") {
            std::cout << "loaded: " << curImage.uri << std::endl;
        }
        else {
            std::cout << "loaded: " << curImage.name << std::endl;
        }
    }
}


void TextureHelper::generateMipmaps(VkCommandBuffer& commandBuffer, VkImage& image, DeviceHelper* pD, int arrayLayers, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(pD->gpu_, imageFormat, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = arrayLayers;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = arrayLayers;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = arrayLayers;

        vkCmdBlitImage(commandBuffer,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);
}

void TextureHelper::createTextureImageView(VkFormat f) {
    pDevHelper_->createImageView(textureImage_, textureImageView_, imageFormat_, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels_);
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
    vkGetPhysicalDeviceProperties(pDevHelper_->gpu_, &properties);

    samplerCInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerCInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerCInfo.mipLodBias = 0.0f;

    if (vkCreateSampler(pDevHelper_->device_, &samplerCInfo, nullptr, &textureSampler_) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create the texture sampler!");
    }
}

void TextureHelper::load() {
    createTextureImages();
    createTextureImageView();
    createTextureImageSampler();
}

TextureHelper::TextureHelper(tinygltf::Model& mod, int32_t textureIndex, DeviceHelper* pD) {
    this->pDevHelper_ = pD;
    this->pInputModel_ = &mod;
    this->mipLevels_ = VK_SAMPLE_COUNT_1_BIT;
    this->texPath_ = "";
    this->index_ = textureIndex;
    this->textureImage_ = VK_NULL_HANDLE;
    this->textureImageMemory_ = VK_NULL_HANDLE;
    this->textureImageView_ = VK_NULL_HANDLE;
    this->textureSampler_ = VK_NULL_HANDLE;
    this->descriptorSet_ = VK_NULL_HANDLE;
}

TextureHelper::TextureHelper(std::string texPath, DeviceHelper* pD) {
    this->pDevHelper_ = pD;
    this->pInputModel_ = nullptr;
    this->mipLevels_ = VK_SAMPLE_COUNT_1_BIT;
    this->texPath_ = texPath;
    this->index_ = INT_MIN;
    this->textureImage_ = VK_NULL_HANDLE;
    this->textureImageMemory_ = VK_NULL_HANDLE;
    this->descriptorSet_ = VK_NULL_HANDLE;
    this->textureImageView_ = VK_NULL_HANDLE;
    this->textureSampler_ = VK_NULL_HANDLE;
};

TextureHelper::~TextureHelper() {
    vkDestroyImage(pDevHelper_->device_, this->textureImage_, nullptr);
    vkDestroyImageView(pDevHelper_->device_, this->textureImageView_, nullptr);
    vkDestroySampler(pDevHelper_->device_, this->textureSampler_, nullptr);
    delete pInputModel_;
    this->pDevHelper_ = nullptr;
}