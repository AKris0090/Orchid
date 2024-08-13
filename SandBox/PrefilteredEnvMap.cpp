#include "PrefilteredEnvMap.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

void PrefilteredEnvMap::createprefEMapImage() {
    pDevHelper_->createSkyBoxImage(width_, height_, mipLevels_, 6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, VK_SAMPLE_COUNT_1_BIT, imageFormat_, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 0, prefEMapImage_, prefEMapImageMemory_);
}

// CODE FROM: https://github.com/SaschaWillems/Vulkan/blob/master/examples/pbrtexture/pbrtexture.cpp
void PrefilteredEnvMap::createprefEMapImageView() {
    VkImageViewCreateInfo prefImageViewCI{};
    prefImageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    prefImageViewCI.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    prefImageViewCI.format = imageFormat_;
    prefImageViewCI.subresourceRange = {};
    prefImageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    prefImageViewCI.subresourceRange.levelCount = mipLevels_;
    prefImageViewCI.subresourceRange.layerCount = 6;
    prefImageViewCI.image = prefEMapImage_;

    vkCreateImageView(pDevHelper_->device_, &prefImageViewCI, nullptr, &prefEMapImageView_);
}

// CODE FROM: https://github.com/SaschaWillems/Vulkan/blob/master/examples/pbrtexture/pbrtexture.cpp
void PrefilteredEnvMap::createprefEMapImageSampler() {
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

    vkCreateSampler(pDevHelper_->device_, &brdfLutImageSamplerCI, nullptr, &prefEMapImageSampler_);

    prefevImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    prefevImageInfo.imageView = prefEMapImageView_;
    prefevImageInfo.sampler = prefEMapImageSampler_;
}

// CODE PARTIALLY FROM: https://github.com/SaschaWillems/Vulkan/blob/master/examples/pbrtexture/pbrtexture.cpp
void PrefilteredEnvMap::createprefEMapDescriptors() {
    VkDescriptorSetLayoutBinding samplerLayoutBindingColor{};
    samplerLayoutBindingColor.binding = 0;
    samplerLayoutBindingColor.descriptorCount = 1;
    samplerLayoutBindingColor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBindingColor.pImmutableSamplers = nullptr;
    samplerLayoutBindingColor.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutCInfo{};
    layoutCInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCInfo.bindingCount = 1;
    layoutCInfo.pBindings = &(samplerLayoutBindingColor);

    vkCreateDescriptorSetLayout(pDevHelper_->device_, &layoutCInfo, nullptr, &prefEMapDescriptorSetLayout_);

    std::array<VkDescriptorPoolSize, 1> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[0].descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolCInfo{};
    poolCInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolCInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolCInfo.pPoolSizes = poolSizes.data();
    poolCInfo.maxSets = 2;

    if (vkCreateDescriptorPool(pDevHelper_->device_, &poolCInfo, nullptr, &prefEMapDescriptorPool_) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create the descriptor pool!");
    }

    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = prefEMapDescriptorPool_;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &prefEMapDescriptorSetLayout_;

    vkAllocateDescriptorSets(pDevHelper_->device_, &allocateInfo, &prefEMapDescriptorSet_);

    VkDescriptorImageInfo skyBoxDescriptorInfo{};
    skyBoxDescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    skyBoxDescriptorInfo.imageView = pSkybox_->skyBoxImageView_;
    skyBoxDescriptorInfo.sampler = pSkybox_->skyBoxImageSampler_;

    VkWriteDescriptorSet prefevDescriptorWriteSet{};
    prefevDescriptorWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    prefevDescriptorWriteSet.dstSet = prefEMapDescriptorSet_;
    prefevDescriptorWriteSet.dstBinding = 0;
    prefevDescriptorWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    prefevDescriptorWriteSet.descriptorCount = 1;
    prefevDescriptorWriteSet.pImageInfo = &skyBoxDescriptorInfo;

    std::vector<VkWriteDescriptorSet> descriptorWriteSets = { prefevDescriptorWriteSet };

    vkUpdateDescriptorSets(pDevHelper_->device_, static_cast<uint32_t>(descriptorWriteSets.size()), descriptorWriteSets.data(), 0, nullptr);
}

void PrefilteredEnvMap::createRenderPass() {
    prefEMapattachment.format = imageFormat_;
    prefEMapattachment.samples = VK_SAMPLE_COUNT_1_BIT;
    prefEMapattachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    prefEMapattachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    prefEMapattachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    prefEMapattachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    prefEMapattachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    prefEMapattachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    VkSubpassDescription subpassDescription = {};
    subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescription.colorAttachmentCount = 1;
    subpassDescription.pColorAttachments = &colorReference;

    std::array<VkSubpassDependency, 2> dependencies;
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
    renderPassCI.pAttachments = &prefEMapattachment;
    renderPassCI.subpassCount = 1;
    renderPassCI.pSubpasses = &subpassDescription;
    renderPassCI.dependencyCount = 2;
    renderPassCI.pDependencies = dependencies.data();

    vkCreateRenderPass(pDevHelper_->device_, &renderPassCI, nullptr, &prefEMapRenderpass_);
}

uint32_t PrefilteredEnvMap::findMemoryType(VkPhysicalDevice gpu_, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(gpu_, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    std::_Xruntime_error("Failed to find a suitable memory type!");
}

void PrefilteredEnvMap::transitionImageLayout(VkCommandBuffer cmdBuff, VkImageSubresourceRange subresourceRange, VkImageLayout oldLayout, VkImageLayout newLayout, VkImage irImage) {
    // transition image layout
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.image = irImage;
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

// CODE PARTIALLY FROM: https://github.com/SaschaWillems/Vulkan/blob/master/examples/pbrtexture/pbrtexture.cpp
void PrefilteredEnvMap::createFrameBuffer() {
    // Offfscreen framebuffer
    {
        // Color attachment
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
        memAlloc.memoryTypeIndex = findMemoryType(pDevHelper_->gpu_, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
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
        fbufCreateInfo.renderPass = prefEMapRenderpass_;
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
        transitionImageLayout(layoutCmd, subRange, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, offscreen.image);
        endCommandBuffer(pDevHelper_->device_, layoutCmd, pGraphicsQueue_, pCommandPool_);
    }
}

std::vector<char> PrefilteredEnvMap::readFile(const std::string& filename) {
    // Start reading at end of the file and read as binary
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        std::cout << "failed to open file" << std::endl;
        std::_Xruntime_error("");
    }

    // Read the file, create the buffer, and return it
    size_t fileSize = file.tellg();
    std::vector<char> buffer((size_t)file.tellg());
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

// In order to pass the binary code to the graphics pipeline, we need to create a VkShaderModule object to wrap it with
VkShaderModule PrefilteredEnvMap::createShaderModule(VkDevice dev, const std::vector<char>& binary) {
    // We need to specify a pointer to the buffer with the bytecode and the length of the bytecode. Bytecode pointer is a uint32_t pointer
    VkShaderModuleCreateInfo shaderModuleCInfo{};
    shaderModuleCInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCInfo.codeSize = binary.size();
    shaderModuleCInfo.pCode = reinterpret_cast<const uint32_t*>(binary.data());

    VkShaderModule shaderMod;
    if (vkCreateShaderModule(dev, &shaderModuleCInfo, nullptr, &shaderMod)) {
        std::_Xruntime_error("Failed to create a shader module!");
    }

    return shaderMod;
}

void PrefilteredEnvMap::createPipeline() {
    VulkanPipelineBuilder::VulkanShaderModule vertexShaderModule = VulkanPipelineBuilder::VulkanShaderModule(pDevHelper_->device_, "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/spv/filterCubeVert.spv");
    VulkanPipelineBuilder::VulkanShaderModule fragmentShaderModule = VulkanPipelineBuilder::VulkanShaderModule(pDevHelper_->device_, "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/spv/prefilteredEnvMapFrag.spv");

    VkPushConstantRange pcRange{};
    pcRange.offset = 0;
    pcRange.size = sizeof(PushBlock);
    pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkShaderModule, 2> shaderStages = { vertexShaderModule.module, fragmentShaderModule.module };

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getPositionAttributeDescription();

    VulkanPipelineBuilder::PipelineBuilderInfo pipelineInfo{};
    pipelineInfo.pDescriptorSetLayouts = &prefEMapDescriptorSetLayout_;
    pipelineInfo.numSets = 1;
    pipelineInfo.pShaderStages = shaderStages.data();
    pipelineInfo.numStages = shaderStages.size();
    pipelineInfo.pPushConstantRanges = &pcRange;
    pipelineInfo.numRanges = 1;
    pipelineInfo.vertexBindingDescriptions = &bindingDescription;
    pipelineInfo.numVertexBindingDescriptions = 1;
    pipelineInfo.vertexAttributeDescriptions = attributeDescriptions.data();
    pipelineInfo.numVertexAttributeDescriptions = static_cast<int>(attributeDescriptions.size());

    prefEMPipeline_ = new VulkanPipelineBuilder(pDevHelper_->device_, pipelineInfo, pDevHelper_);

    prefEMPipeline_->info.pColorBlendState->attachmentCount = 1;
    prefEMPipeline_->info.pMultisampleState->rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    prefEMPipeline_->info.pDepthStencilState->back.compareOp = VK_COMPARE_OP_ALWAYS;
    prefEMPipeline_->info.pDepthStencilState->depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    prefEMPipeline_->info.pDepthStencilState->depthTestEnable = VK_FALSE;
    prefEMPipeline_->info.pRasterizationState->cullMode = VK_CULL_MODE_NONE;

    prefEMPipeline_->generate(pipelineInfo, prefEMapRenderpass_);
}


void PrefilteredEnvMap::endCommandBuffer(VkDevice device_, VkCommandBuffer cmdBuff, VkQueue* pGraphicsQueue_, VkCommandPool* pCommandPool_) {
    vkEndCommandBuffer(cmdBuff);

    VkSubmitInfo queueSubmitInfo{};
    queueSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    queueSubmitInfo.commandBufferCount = 1;
    queueSubmitInfo.pCommandBuffers = &cmdBuff;
    // Create fence to ensure that the command buffer has finished executing
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence fence;
    vkCreateFence(device_, &fenceInfo, nullptr, &fence);
    // Submit to the queue
    vkQueueSubmit(*(pGraphicsQueue_), 1, &queueSubmitInfo, fence);
    // Wait for the fence to signal that command buffer has finished executing
    vkWaitForFences(device_, 1, &fence, VK_TRUE, 100000000000); // big number is fence timeout
    vkDestroyFence(device_, fence, nullptr);
    
    vkFreeCommandBuffers(device_, *(pCommandPool_), 1, &cmdBuff);
}

// CODE PARTIALLY FROM: https://github.com/SaschaWillems/Vulkan/blob/master/examples/pbrtexture/pbrtexture.cpp
void PrefilteredEnvMap::render(VkBuffer& vertexBuffer, VkBuffer& indexBuffer) {
    VkClearValue clearValues[1];
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 0.0f } };

    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = prefEMapRenderpass_;
    renderPassBeginInfo.renderArea.extent.width = width_;
    renderPassBeginInfo.renderArea.extent.height = height_;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = clearValues;
    renderPassBeginInfo.framebuffer = offscreen.framebuffer;

    glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 512.0f);

    std::vector<glm::mat4> matrices = { 
        glm::lookAt(glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), glm::vec3(0, -1, 0)),
        glm::lookAt(glm::vec3(0, 0, 0), glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0)),
        glm::lookAt(glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)),
        glm::lookAt(glm::vec3(0, 0, 0), glm::vec3(0, -1, 0), glm::vec3(0, 0, -1)),
        glm::lookAt(glm::vec3(0, 0, 0), glm::vec3(0, 0, 1), glm::vec3(0, -1, 0)),
        glm::lookAt(glm::vec3(0, 0, 0), glm::vec3(0, 0, -1), glm::vec3(0, -1, 0))};

    VkCommandBuffer cmdBuf = pDevHelper_->beginSingleTimeCommands();

    VkImageSubresourceRange subresourceRange = {};
    subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange.baseMipLevel = 0;
    subresourceRange.levelCount = mipLevels_;
    subresourceRange.layerCount = 6;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    viewport.width = (float) width_;
    viewport.height = (float) height_;
    vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent.width = width_;
    scissor.extent.height = height_;
    vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

    transitionImageLayout(cmdBuf, subresourceRange, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, prefEMapImage_);
    VkImageSubresourceRange subresourceRange2 = {};
    subresourceRange2.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subresourceRange2.baseMipLevel = 0;
    subresourceRange2.levelCount = 1;
    subresourceRange2.layerCount = 1;

    VkBuffer vertexBuffers[] = { vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(cmdBuf, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    for (int mip = 0; mip < mipLevels_; mip++) {
        pushBlock.roughness = (float)mip / (float)(mipLevels_ - 1);
        for (int face = 0; face < 6; face++) {
            viewport.width = static_cast<float>(width_ * std::pow(0.5f, mip));
            viewport.height = static_cast<float>(height_ * std::pow(0.5f, mip));
            vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

            vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

            pushBlock.mvp = proj * matrices[face];

            vkCmdPushConstants(cmdBuf, prefEMPipeline_->layout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushBlock), &pushBlock);

            vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, prefEMPipeline_->pipeline);
            vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, prefEMPipeline_->layout , 0, 1, &prefEMapDescriptorSet_, 0, NULL);

            pSkybox_->pSkyBoxModel_->drawSkyBoxIndexed(cmdBuf);

            vkCmdEndRenderPass(cmdBuf);

            transitionImageLayout(cmdBuf, subresourceRange2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, offscreen.image);

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

            vkCmdCopyImage(cmdBuf, offscreen.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, prefEMapImage_, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

            transitionImageLayout(cmdBuf, subresourceRange2, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, offscreen.image);
        }
    }

    transitionImageLayout(cmdBuf, subresourceRange, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, prefEMapImage_);

    endCommandBuffer(pDevHelper_->device_, cmdBuf, pGraphicsQueue_, pCommandPool_);
}

void PrefilteredEnvMap::genprefEMap(VkBuffer& vertexBuffer, VkBuffer& indexBuffer) {
    createprefEMapImage();
    createprefEMapImageView();
    createprefEMapImageSampler();

    createRenderPass();
    createFrameBuffer();

    createprefEMapDescriptors();

    createPipeline();
    render(vertexBuffer, indexBuffer);
}

PrefilteredEnvMap::PrefilteredEnvMap(DeviceHelper* devHelper, VkQueue* graphicsQueue, VkCommandPool* cmdPool, Skybox* pSkybox) {
    this->pDevHelper_ = devHelper;
    this->imageFormat_ = VK_FORMAT_R16G16B16A16_SFLOAT;
    this->width_ = this->height_ = 512;
    this->mipLevels_ = static_cast<uint32_t>(floor(log2(512))) + 1;
    this->pGraphicsQueue_ = graphicsQueue;
    this->pSkybox_ = pSkybox;
    this->pCommandPool_ = cmdPool;
}