#include "Skybox.h"

void Skybox::transitionImageLayout(VkCommandBuffer cmdBuff, VkImageSubresourceRange subresourceRange, VkImageLayout oldLayout, VkImageLayout newLayout) {
    // transition image layout
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = skyBoxImage_;
    barrier.subresourceRange = subresourceRange;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    barrier.srcAccessMask = 0;  
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

    vkCmdPipelineBarrier(cmdBuff, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    std::cout << "transitioned skybox image" << std::endl;
}

void Skybox::copyBufferToImage(VkCommandBuffer cmdBuff, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 6;

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height, 1 };

    vkCmdCopyBufferToImage(cmdBuff, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    std::cout << "created texture image" << std::endl;
}

void Skybox::createSkyBoxImage() {
    VkDeviceSize buffSize = 0;
    int texWidth, texHeight, texChannels;
    VkDeviceSize imageSize;
    imageFormat_ = VK_FORMAT_R8G8B8A8_SRGB;

    for (int i = 0; i < 6; i++) {
        pixels[i] = stbi_load(texPaths_[i].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    }

    imageSize = texWidth * texHeight * 4;
    size_t totalImageSize = imageSize * 6;

    mipLevels_ = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    if (!pixels) {
        throw std::runtime_error("failed to load skybox image!");
    }

    pDevHelper_->createBuffer(totalImageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer_, stagingBufferMemory_);

    void* data;
    for (int i = 0; i < 6; i++) {
        vkMapMemory(device_, stagingBufferMemory_, (i * imageSize), imageSize, 0, &data);
        memcpy(data, pixels[i], static_cast<size_t>(imageSize));
        vkUnmapMemory(pDevHelper_->getDevice(), stagingBufferMemory_);
    }

    pDevHelper_->createSkyBoxImage(texWidth, texHeight, mipLevels_, 6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, skyBoxImage_, skyBoxImageMemory_);

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = mipLevels_;
    subresourceRange.layerCount = 6;

    VkCommandBuffer copyCommandBuffer = pDevHelper_->beginSingleTimeCommands();

    transitionImageLayout(copyCommandBuffer, subresourceRange, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    
    copyBufferToImage(copyCommandBuffer, stagingBuffer_, skyBoxImage_, texWidth, texHeight);

    transitionImageLayout(copyCommandBuffer, subresourceRange, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    pDevHelper_->endSingleTimeCommands(copyCommandBuffer);
}

void Skybox::createSkyBoxImageView() {
    // Create image view
    VkImageViewCreateInfo imageViewCInfo{};
    imageViewCInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCInfo.image = skyBoxImage_;
    imageViewCInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCInfo.format = imageFormat_;

    imageViewCInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    imageViewCInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    imageViewCInfo.subresourceRange.layerCount = 6;
    imageViewCInfo.subresourceRange.levelCount = mipLevels_;

    vkCreateImageView(device_, &imageViewCInfo, nullptr, &skyBoxImageView_);
}

void Skybox::createSkyBoxImageSampler() {
    // create sampler
    VkSamplerCreateInfo samplerCInfo{};
    samplerCInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCInfo.magFilter = VK_FILTER_NEAREST;
    samplerCInfo.minFilter = VK_FILTER_LINEAR;
    samplerCInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCInfo.anisotropyEnable = VK_TRUE;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(pDevHelper_->getPhysicalDevice(), &properties);
    samplerCInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerCInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    samplerCInfo.unnormalizedCoordinates = VK_FALSE;
    samplerCInfo.compareEnable = VK_FALSE;
    samplerCInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerCInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCInfo.mipLodBias = 0.0f;
    samplerCInfo.minLod = 0.0f;
    samplerCInfo.maxLod = 0.0f;

    if (vkCreateSampler(pDevHelper_->getDevice(), &samplerCInfo, nullptr, &skyBoxImageSampler_) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create the texture sampler!");
    }
}

void Skybox::skyBoxLoad() {
    createSkyBoxImage();
    createSkyBoxImageView();
    createSkyBoxImageSampler();
    vkDestroyBuffer(pDevHelper_->getDevice(), stagingBuffer_, nullptr);
    vkFreeMemory(pDevHelper_->getDevice(), stagingBufferMemory_, nullptr);
    createSkyBoxDescriptorSetLayout();
    createDescriptorSet();
}

void Skybox::createSkyBoxDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding samplerLayoutBindingColor{};
    samplerLayoutBindingColor.binding = 0;
    samplerLayoutBindingColor.descriptorCount = 1;
    samplerLayoutBindingColor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBindingColor.pImmutableSamplers = nullptr;
    samplerLayoutBindingColor.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutCInfo{};
    layoutCInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCInfo.bindingCount = 1;
    layoutCInfo.pBindings = &samplerLayoutBindingColor;

    if (vkCreateDescriptorSetLayout(device_, &layoutCInfo, nullptr, &skyBoxDescriptorSetLayout_) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create the uniform descriptor set layout!");
    }
}

void Skybox::createDescriptorSet() {
    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = pDevHelper_->getDescriptorPool();
    allocateInfo.descriptorSetCount = 1;
    VkDescriptorSetLayout skySet = skyBoxDescriptorSetLayout_;
    allocateInfo.pSetLayouts = &(skySet);

    VkResult res = vkAllocateDescriptorSets(pDevHelper_->getDevice(), &allocateInfo, &(skyBoxDescriptorSet_));
    if (res != VK_SUCCESS) {
        std::_Xruntime_error("Failed to allocate descriptor sets!");
    }

    VkDescriptorImageInfo skyBoxDescriptorInfo{};
    skyBoxDescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    skyBoxDescriptorInfo.imageView = skyBoxImageView_;
    skyBoxDescriptorInfo.sampler = skyBoxImageSampler_;

    VkWriteDescriptorSet skyBoxDescriptorWrite {};
    skyBoxDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    skyBoxDescriptorWrite.dstSet = skyBoxDescriptorSet_;
    skyBoxDescriptorWrite.dstBinding = 0;
    skyBoxDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    skyBoxDescriptorWrite.descriptorCount = 1;
    skyBoxDescriptorWrite.pImageInfo = &skyBoxDescriptorInfo;

    vkUpdateDescriptorSets(device_, 1, &skyBoxDescriptorWrite, 0, nullptr);
}

Skybox::Skybox(std::string modPath, std::vector<std::string> texPaths, DeviceHelper* devHelper) {
	this->modPath_ = modPath;
	this->texPaths_ = texPaths;
	this->pDevHelper_ = devHelper;
    this->device_ = devHelper->getDevice();
}

void Skybox::loadSkyBox() {
	this->pSkyBoxModel_ = new GLTFObj(modPath_, pDevHelper_);
	pSkyBoxModel_->loadGLTF();
    pSkyBoxModel_->isSkyBox_ = true;
    skyBoxLoad();
}