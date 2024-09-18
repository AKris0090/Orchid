#include "IrradianceCube.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

void IrradianceCube::createiRCubeImage() {
    pDevHelper_->createImage(width_, height_, mipLevels_, 6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, VK_SAMPLE_COUNT_1_BIT, imageFormat_, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 0, iRCubeImage_, iRCubeImageMemory_);
}

// CODE FROM: https://github.com/SaschaWillems/Vulkan/blob/master/examples/pbrtexture/pbrtexture.cpp
void IrradianceCube::createiRCubeImageView() {
    VkImageViewCreateInfo iRImageViewCI{};
    iRImageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    iRImageViewCI.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    iRImageViewCI.format = imageFormat_;
    iRImageViewCI.subresourceRange = {};
    iRImageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    iRImageViewCI.subresourceRange.levelCount = mipLevels_;
    iRImageViewCI.subresourceRange.layerCount = 6;
    iRImageViewCI.image = iRCubeImage_;

    vkCreateImageView(pDevHelper_->device_, &iRImageViewCI, nullptr, &iRCubeImageView_);
}

// CODE FROM: https://github.com/SaschaWillems/Vulkan/blob/master/examples/pbrtexture/pbrtexture.cpp
void IrradianceCube::createiRCubeImageSampler() {
    VkSamplerCreateInfo brdfLutImageSamplerCI{};
    brdfLutImageSamplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    brdfLutImageSamplerCI.magFilter = VK_FILTER_LINEAR;
    brdfLutImageSamplerCI.minFilter = VK_FILTER_LINEAR;
    brdfLutImageSamplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    brdfLutImageSamplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    brdfLutImageSamplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    brdfLutImageSamplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    brdfLutImageSamplerCI.minLod = 0.0f;
    brdfLutImageSamplerCI.maxLod = static_cast<float>(mipLevels_);
    brdfLutImageSamplerCI.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    vkCreateSampler(pDevHelper_->device_, &brdfLutImageSamplerCI, nullptr, &iRCubeImageSampler_);

    irImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    irImageInfo.imageView = iRCubeImageView_;
    irImageInfo.sampler = iRCubeImageSampler_;
}

// CODE PARTIALLY FROM: https://github.com/SaschaWillems/Vulkan/blob/master/examples/pbrtexture/pbrtexture.cpp
void IrradianceCube::createiRCubeDescriptors() {
    std::vector<VulkanDescriptorLayoutBuilder::BindingStruct> binding{};
    binding.push_back(VulkanDescriptorLayoutBuilder::BindingStruct{
                .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .stageBits = VK_SHADER_STAGE_FRAGMENT_BIT
        });

    iRCubeDescriptorSetLayout_ = new VulkanDescriptorLayoutBuilder(pDevHelper_, binding);

    std::array<VkDescriptorPoolSize, 1> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[0].descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolCInfo{};
    poolCInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolCInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolCInfo.pPoolSizes = poolSizes.data();
    poolCInfo.maxSets = 2;

    if (vkCreateDescriptorPool(pDevHelper_->device_, &poolCInfo, nullptr, &iRCubeDescriptorPool_) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create the descriptor pool!");
    }

    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = iRCubeDescriptorPool_;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &iRCubeDescriptorSetLayout_->layout;

    vkAllocateDescriptorSets(pDevHelper_->device_, &allocateInfo, &iRCubeDescriptorSet_);

    // WRITE SET

    VkDescriptorImageInfo skyBoxDescriptorInfo{};
    skyBoxDescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    skyBoxDescriptorInfo.imageView = pSkybox_->skyBoxImageView_;
    skyBoxDescriptorInfo.sampler = pSkybox_->skyBoxImageSampler_;

    VkWriteDescriptorSet irDescriptorWriteSet{};
    irDescriptorWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    irDescriptorWriteSet.dstSet = iRCubeDescriptorSet_;
    irDescriptorWriteSet.dstBinding = 0;
    irDescriptorWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    irDescriptorWriteSet.descriptorCount = 1;
    irDescriptorWriteSet.pImageInfo = &skyBoxDescriptorInfo;

    std::vector<VkWriteDescriptorSet> descriptorWriteSets = { irDescriptorWriteSet };

    vkUpdateDescriptorSets(pDevHelper_->device_, static_cast<uint32_t>(descriptorWriteSets.size()), descriptorWriteSets.data(), 0, nullptr);
}

void IrradianceCube::createRenderPass() {
    iRCubeattachment.format = imageFormat_;
    iRCubeattachment.samples = VK_SAMPLE_COUNT_1_BIT;
    iRCubeattachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    iRCubeattachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    iRCubeattachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    iRCubeattachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    iRCubeattachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    iRCubeattachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorReference;

    std::array<VkSubpassDependency, 2> dependencies{};
    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassCI{};
    renderPassCI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCI.attachmentCount = 1;
    renderPassCI.pAttachments = &iRCubeattachment;
    renderPassCI.subpassCount = 1;
    renderPassCI.pSubpasses = &subpassDescription;
    renderPassCI.dependencyCount = 2;
    renderPassCI.pDependencies = dependencies.data();

    vkCreateRenderPass(pDevHelper_->device_, &renderPassCI, nullptr, &iRCubeRenderpass_);
}

// CODE PARTIALLY FROM: https://github.com/SaschaWillems/Vulkan/blob/master/examples/pbrtexture/pbrtexture.cpp
void IrradianceCube::createFrameBuffer() {
    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = imageFormat_;
    imageCreateInfo.extent.width = width_;
    imageCreateInfo.extent.height = height_;
    imageCreateInfo.extent.depth = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreateInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    vkCreateImage(pDevHelper_->device_, &imageCreateInfo, nullptr, &offscreen.image);

    VkMemoryAllocateInfo memAlloc{};
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(pDevHelper_->device_, offscreen.image, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = pDevHelper_->findMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    vkAllocateMemory(pDevHelper_->device_, &memAlloc, nullptr, &offscreen.memory);
    vkBindImageMemory(pDevHelper_->device_, offscreen.image, offscreen.memory, 0);

    VkImageViewCreateInfo colorImageView{};
    colorImageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
    colorImageView.format = imageFormat_;
    colorImageView.flags = 0;
    colorImageView.subresourceRange = {};
    colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    colorImageView.subresourceRange.baseMipLevel = 0;
    colorImageView.subresourceRange.levelCount = 1;
    colorImageView.subresourceRange.baseArrayLayer = 0;
    colorImageView.subresourceRange.layerCount = 1;
    colorImageView.image = offscreen.image;
    vkCreateImageView(pDevHelper_->device_, &colorImageView, nullptr, &offscreen.view);

    VkFramebufferCreateInfo fbufCreateInfo{};
    fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbufCreateInfo.renderPass = iRCubeRenderpass_;
    fbufCreateInfo.attachmentCount = 1;
    fbufCreateInfo.pAttachments = &offscreen.view;
    fbufCreateInfo.width = width_;
    fbufCreateInfo.height = height_;
    fbufCreateInfo.layers = 1;
    vkCreateFramebuffer(pDevHelper_->device_, &fbufCreateInfo, nullptr, &offscreen.framebuffer);

    VkCommandBuffer layoutCmd = pDevHelper_->beginSingleTimeCommands();
    VkImageSubresourceRange subRange{};
    subRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subRange.baseMipLevel = 0;
    subRange.levelCount = 1;
    subRange.layerCount = 1;
    pDevHelper_->transitionImageLayout(layoutCmd, subRange, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, offscreen.image);
    pDevHelper_->protectedEndCommands(layoutCmd);
}

void IrradianceCube::createPipeline() {
    VulkanPipelineBuilder::VulkanShaderModule vertexShaderModule = VulkanPipelineBuilder::VulkanShaderModule(pDevHelper_->device_, "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/spv/filterCubeVert.spv");
    VulkanPipelineBuilder::VulkanShaderModule fragmentShaderModule = VulkanPipelineBuilder::VulkanShaderModule(pDevHelper_->device_, "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/spv/irradianceCubeFrag.spv");

    VkPushConstantRange pcRange{};
    pcRange.offset = 0;
    pcRange.size = sizeof(PushBlock);
    pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VulkanPipelineBuilder::VulkanShaderModule, 2> shaderStages = { vertexShaderModule, fragmentShaderModule };

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getPositionAttributeDescription();

    VulkanPipelineBuilder::PipelineBuilderInfo pipelineInfo{};
    pipelineInfo.pDescriptorSetLayouts = &iRCubeDescriptorSetLayout_->layout;
    pipelineInfo.numSets = 1;
    pipelineInfo.pShaderStages = shaderStages.data();
    pipelineInfo.numStages = static_cast<int>(shaderStages.size());
    pipelineInfo.pPushConstantRanges = &pcRange;
    pipelineInfo.numRanges = 1;
    pipelineInfo.vertexBindingDescriptions = &bindingDescription;
    pipelineInfo.numVertexBindingDescriptions = 1;
    pipelineInfo.vertexAttributeDescriptions = attributeDescriptions.data();
    pipelineInfo.numVertexAttributeDescriptions = static_cast<int>(attributeDescriptions.size());

    iRPipeline_ = new VulkanPipelineBuilder(pDevHelper_->device_, pipelineInfo, pDevHelper_);

    iRPipeline_->info.pColorBlendState->attachmentCount = 1;
    iRPipeline_->info.pMultisampleState->rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    iRPipeline_->info.pDepthStencilState->back.compareOp = VK_COMPARE_OP_ALWAYS;
    iRPipeline_->info.pDepthStencilState->depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    iRPipeline_->info.pDepthStencilState->depthTestEnable = VK_FALSE;
    iRPipeline_->info.pRasterizationState->cullMode = VK_CULL_MODE_NONE;

    iRPipeline_->generate(pipelineInfo, iRCubeRenderpass_);
}

// CODE PARTIALLY FROM: https://github.com/SaschaWillems/Vulkan/blob/master/examples/pbrtexture/pbrtexture.cpp
void IrradianceCube::render(VkBuffer& vertexBuffer, VkBuffer& indexBuffer) {
    VkClearValue clearValues[1]{};
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };

    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = iRCubeRenderpass_;
    renderPassBeginInfo.renderArea.extent.width = width_;
    renderPassBeginInfo.renderArea.extent.height = height_;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = clearValues;
    renderPassBeginInfo.framebuffer = offscreen.framebuffer;

    VkCommandBuffer cmdBuf = pDevHelper_->beginSingleTimeCommands();

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = mipLevels_;
    subresourceRange.layerCount = 6;

    VkViewport viewport{};
    viewport.width = float(width_);
    viewport.height = float(height_);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent.width = width_;
    scissor.extent.height = height_;
    vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

    VkBuffer vertexBuffers[] = { vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmdBuf, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    pDevHelper_->transitionImageLayout(cmdBuf, subresourceRange, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, iRCubeImage_);

    VkImageSubresourceRange subresourceRange2 = {};
    subresourceRange2.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange2.baseMipLevel = 0;
    subresourceRange2.levelCount = 1;
    subresourceRange2.layerCount = 1;

    glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 512.0f);

    std::vector<glm::mat4> matrices = { 
        glm::lookAt(glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), glm::vec3(0, -1, 0)),
        glm::lookAt(glm::vec3(0, 0, 0), glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0)),
        glm::lookAt(glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)),
        glm::lookAt(glm::vec3(0, 0, 0), glm::vec3(0, -1, 0), glm::vec3(0, 0, -1)),
        glm::lookAt(glm::vec3(0, 0, 0), glm::vec3(0, 0, 1), glm::vec3(0, -1, 0)),
        glm::lookAt(glm::vec3(0, 0, 0), glm::vec3(0, 0, -1), glm::vec3(0, -1, 0)) };

    for (uint32_t  mip = 0; mip < mipLevels_; mip++) {
        for (uint32_t  face = 0; face < 6; face++) {
            viewport.width = static_cast<float>(width_ * std::pow(0.5f, mip));
            viewport.height = static_cast<float>(height_ * std::pow(0.5f, mip));
            vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

            vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE); 

            pushBlock.mvp = proj * matrices[face];

            vkCmdPushConstants(cmdBuf, iRPipeline_->layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(IrradianceCube::PushBlock), &pushBlock);

            vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, iRPipeline_->pipeline);
            vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, iRPipeline_->layout, 0, 1, &iRCubeDescriptorSet_, 0, NULL);

            pSkybox_->drawSkyBoxIndexed(cmdBuf);

            vkCmdEndRenderPass(cmdBuf);

            pDevHelper_->transitionImageLayout(cmdBuf, subresourceRange2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, offscreen.image);
            
            // Copy region for transfer from framebuffer to cube face - https://github.com/SaschaWillems/Vulkan/blob/master/examples/pbrtexture/pbrtexture.cpp
            VkImageCopy copyRegion = {};

            copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.srcSubresource.baseArrayLayer = 0;
            copyRegion.srcSubresource.mipLevel = 0;
            copyRegion.srcSubresource.layerCount = 1;
            copyRegion.srcOffset = { 0, 0, 0 };

            copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyRegion.dstSubresource.baseArrayLayer = face;
            copyRegion.dstSubresource.mipLevel = mip;
            copyRegion.dstSubresource.layerCount = 1;
            copyRegion.dstOffset = { 0, 0, 0 };

            copyRegion.extent.width = static_cast<uint32_t>(viewport.width);
            copyRegion.extent.height = static_cast<uint32_t>(viewport.height);
            copyRegion.extent.depth = 1;

            vkCmdCopyImage(cmdBuf, offscreen.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, iRCubeImage_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

            pDevHelper_->transitionImageLayout(cmdBuf, subresourceRange2, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, offscreen.image);
        }
    }

    pDevHelper_->transitionImageLayout(cmdBuf, subresourceRange, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, iRCubeImage_);

    pDevHelper_->protectedEndCommands(cmdBuf);

    vkDestroyImageView(this->pDevHelper_->device_, offscreen.view, nullptr);
    vkDestroyImage(this->pDevHelper_->device_, offscreen.image, nullptr);
    vkFreeMemory(this->pDevHelper_->device_, offscreen.memory, nullptr);
    vkDestroyFramebuffer(this->pDevHelper_->device_, offscreen.framebuffer, nullptr);
}

void IrradianceCube::geniRCube(VkBuffer& vertexBuffer, VkBuffer& indexBuffer) {
    createiRCubeImage();
    createiRCubeImageView();
    createiRCubeImageSampler();

    createRenderPass();
    createFrameBuffer();

    createiRCubeDescriptors();

    createPipeline();
    render(vertexBuffer, indexBuffer);
}

IrradianceCube::IrradianceCube(DeviceHelper* devHelper, Skybox* pSkybox, VkBuffer& vertexBuffer, VkBuffer& indexBuffer) {
    this->pDevHelper_ = devHelper;
    this->imageFormat_ = VK_FORMAT_R32G32B32A32_SFLOAT;
    this->width_ = this->height_ = 64;
    this->mipLevels_ = static_cast<uint32_t>(floor(log2(64))) + 1;
    this->pSkybox_ = pSkybox;
    this->iRCubeImage_ = VK_NULL_HANDLE;
    this->iRCubeFrameBuffer_ = VK_NULL_HANDLE;
    this->iRCubeRenderpass_ = VK_NULL_HANDLE;
    this->iRCubeDescriptorSetLayout_ = VK_NULL_HANDLE;
    this->iRCubeDescriptorPool_ = VK_NULL_HANDLE;
    this->iRCubeDescriptorSet_ = VK_NULL_HANDLE;
    this->iRCubeImageMemory_ = VK_NULL_HANDLE;
    this->iRCubeImageView_ = VK_NULL_HANDLE;
    this->iRCubeImageSampler_ = VK_NULL_HANDLE;
    this->offscreen = OffscreenStruct{};
    this->iRCubeattachment = VkAttachmentDescription{};
    this->irImageInfo = VkDescriptorImageInfo{};
    this->iRPipeline_ = nullptr;

    geniRCube(vertexBuffer, indexBuffer);

    preDelete();
}

void IrradianceCube::preDelete() {
    vkDestroyFramebuffer(this->pDevHelper_->device_, this->iRCubeFrameBuffer_, nullptr);
    vkDestroyRenderPass(this->pDevHelper_->device_, this->iRCubeRenderpass_, nullptr);
    delete iRCubeDescriptorSetLayout_;
    delete iRPipeline_;
    this->pDevHelper_ = nullptr;
}

IrradianceCube::~IrradianceCube() {
    vkDestroySampler(this->pDevHelper_->device_, this->iRCubeImageSampler_, nullptr);
    vkDestroyImageView(this->pDevHelper_->device_, this->iRCubeImageView_, nullptr);
    vkDestroyImage(this->pDevHelper_->device_, this->iRCubeImage_, nullptr);
    vkFreeMemory(this->pDevHelper_->device_, iRCubeImageMemory_, nullptr);
    vkDestroyDescriptorPool(this->pDevHelper_->device_, this->iRCubeDescriptorPool_, nullptr);
}