#include "Skybox.h"

void Skybox::drawSkyBoxIndexed(VkCommandBuffer& commandBuffer) {
    MeshHelper* m = (pSkyBoxModel_->pParentNodes[0])->meshPrimitives[0];
    MeshHelper::callIndexedDraw(commandBuffer, m->indirectInfo);
}

void Skybox::createSkyBoxImage() {
    VkDeviceSize buffSize = 0;
    int texWidth, texHeight, texChannels;
    VkDeviceSize imageSize;
    imageFormat_ = VK_FORMAT_R8G8B8A8_SRGB;

    for (int i = 0; i < 6; i++) {
        pixels[i] = stbi_load(texPaths_[i].c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    }

    imageSize = static_cast<VkDeviceSize>(texWidth) * texHeight * 4;
    size_t totalImageSize = imageSize * 6;

    mipLevels_ = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    if (!pixels) {
        throw std::runtime_error("failed to load skybox image!");
    }

    pDevHelper_->createBuffer(totalImageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer_, stagingBufferMemory_);

    void* data;
    for (int i = 0; i < 6; i++) {
        vkMapMemory(pDevHelper_->device_, stagingBufferMemory_, (i * imageSize), imageSize, 0, &data);
        memcpy(data, pixels[i], static_cast<size_t>(imageSize));
        vkUnmapMemory(pDevHelper_->device_, stagingBufferMemory_);
    }

    pDevHelper_->createImage(texWidth, texHeight, mipLevels_, 6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, VK_SAMPLE_COUNT_1_BIT, imageFormat_, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, skyBoxImage_, skyBoxImageMemory_);

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = mipLevels_;
    subresourceRange.baseArrayLayer = 0;
    subresourceRange.layerCount = 6;

    VkCommandBuffer copyCommandBuffer = pDevHelper_->beginSingleTimeCommands();
    pDevHelper_->transitionImageLayout(copyCommandBuffer, subresourceRange, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, skyBoxImage_);
    TextureHelper::copyBufferToImage(copyCommandBuffer, stagingBuffer_, skyBoxImage_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, pDevHelper_, 6, texWidth, texHeight);

    TextureHelper::generateMipmaps(copyCommandBuffer, skyBoxImage_, pDevHelper_, 6, imageFormat_, texWidth, texHeight, this->mipLevels_);
    pDevHelper_->endSingleTimeCommands(copyCommandBuffer);

    vkDestroyBuffer(pDevHelper_->device_, stagingBuffer_, nullptr);
    vkFreeMemory(pDevHelper_->device_, stagingBufferMemory_, nullptr);

    //delete[] pixels;
}

void Skybox::createSkyBoxImageView() {
    VkImageViewCreateInfo imageViewCInfo{};
    imageViewCInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCInfo.image = skyBoxImage_;
    imageViewCInfo.format = imageFormat_;

    imageViewCInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    imageViewCInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
    imageViewCInfo.subresourceRange.layerCount = 6;
    imageViewCInfo.subresourceRange.levelCount = mipLevels_;

    vkCreateImageView(pDevHelper_->device_, &imageViewCInfo, nullptr, &skyBoxImageView_);
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
    vkGetPhysicalDeviceProperties(pDevHelper_->gpu_, &properties);
    samplerCInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerCInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    samplerCInfo.unnormalizedCoordinates = VK_FALSE;
    samplerCInfo.compareEnable = VK_FALSE;
    samplerCInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerCInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCInfo.mipLodBias = 0.0f;
    samplerCInfo.minLod = 0.0f;
    samplerCInfo.maxLod = this->mipLevels_;

    if (vkCreateSampler(pDevHelper_->device_, &samplerCInfo, nullptr, &skyBoxImageSampler_) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create the texture sampler!");
    }
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

    if (vkCreateDescriptorSetLayout(pDevHelper_->device_, &layoutCInfo, nullptr, &skyBoxDescriptorSetLayout_) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create the uniform descriptor set layout!");
    }
}

void Skybox::createDescriptorSet() {
    std::array<VkDescriptorPoolSize, 1> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[0].descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolCInfo{};
    poolCInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolCInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolCInfo.pPoolSizes = poolSizes.data();
    poolCInfo.maxSets = 1;

    if (vkCreateDescriptorPool(pDevHelper_->device_, &poolCInfo, nullptr, &skyBoxDescriptorPool_) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create the descriptor pool!");
    }

    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = skyBoxDescriptorPool_;
    allocateInfo.descriptorSetCount = 1;
    VkDescriptorSetLayout skySet = skyBoxDescriptorSetLayout_;
    allocateInfo.pSetLayouts = &(skySet);

    VkResult res = vkAllocateDescriptorSets(pDevHelper_->device_, &allocateInfo, &(skyBoxDescriptorSet_));
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

    vkUpdateDescriptorSets(pDevHelper_->device_, 1, &skyBoxDescriptorWrite, 0, nullptr);
}

void Skybox::loadSkyBox(uint32_t globalVertexOffset, uint32_t globalIndexOffset) {
	this->pSkyBoxModel_ = new GLTFObj(modPath_, pDevHelper_, globalVertexOffset, globalIndexOffset);
    pSkyBoxModel_->isSkyBox_ = true;
    
    createSkyBoxImage();
    createSkyBoxImageView();
    createSkyBoxImageSampler();

    createSkyBoxDescriptorSetLayout();
    createDescriptorSet();
}

Skybox::Skybox(std::string modPath, std::vector<std::string> texPaths, DeviceHelper* devHelper, uint32_t globalVertexOffset, uint32_t globalIndexOffset) {
    this->modPath_ = modPath;
    this->texPaths_ = texPaths;
    this->pDevHelper_ = devHelper;

    loadSkyBox(globalVertexOffset, globalIndexOffset);
}

Skybox::~Skybox() {
    vkDestroySampler(pDevHelper_->device_, this->skyBoxImageSampler_, nullptr);
    vkDestroyImageView(pDevHelper_->device_, this->skyBoxImageView_, nullptr);
    vkDestroyImage(pDevHelper_->device_, this->skyBoxImage_, nullptr);
    vkFreeMemory(pDevHelper_->device_, skyBoxImageMemory_, nullptr);
    vkDestroyDescriptorSetLayout(pDevHelper_->device_, this->skyBoxDescriptorSetLayout_, nullptr);
    vkDestroyDescriptorPool(pDevHelper_->device_, this->skyBoxDescriptorPool_, nullptr);
    delete skyBoxPipeline_;
    delete this->pSkyBoxModel_;
    this->pDevHelper_ = nullptr;
}