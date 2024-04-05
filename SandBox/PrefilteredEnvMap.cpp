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

    vkCreateImageView(device_, &prefImageViewCI, nullptr, &prefEMapImageView_);
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

    vkCreateSampler(device_, &brdfLutImageSamplerCI, nullptr, &prefEMapImageSampler_);

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

    vkCreateDescriptorSetLayout(device_, &layoutCInfo, nullptr, &prefEMapDescriptorSetLayout_);

    std::array<VkDescriptorPoolSize, 1> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[0].descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolCInfo{};
    poolCInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolCInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolCInfo.pPoolSizes = poolSizes.data();
    poolCInfo.maxSets = 2;

    if (vkCreateDescriptorPool(device_, &poolCInfo, nullptr, &prefEMapDescriptorPool_) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create the descriptor pool!");
    }

    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = prefEMapDescriptorPool_;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &prefEMapDescriptorSetLayout_;

    vkAllocateDescriptorSets(device_, &allocateInfo, &prefEMapDescriptorSet_);

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

    vkUpdateDescriptorSets(pDevHelper_->getDevice(), static_cast<uint32_t>(descriptorWriteSets.size()), descriptorWriteSets.data(), 0, nullptr);
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

    vkCreateRenderPass(device_, &renderPassCI, nullptr, &prefEMapRenderpass_);
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

void PrefilteredEnvMap::transitionImageLayout(VkCommandBuffer cmdBuff, VkImageSubresourceRange subresourceRange, VkImageLayout oldLayout, VkImageLayout newLayout, VkImage prefImage) {
    // transition image layout
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = prefImage;
    barrier.subresourceRange = subresourceRange;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    vkCmdPipelineBarrier(cmdBuff, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    std::cout << "transitioned skybox image" << std::endl;
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
        vkCreateImage(device_, &imageCreateInfo, nullptr, &offscreen.image);

        VkMemoryAllocateInfo memAlloc{};
        memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(device_, offscreen.image, &memReqs);
        memAlloc.allocationSize = memReqs.size;
        memAlloc.memoryTypeIndex = findMemoryType(pDevHelper_->getPhysicalDevice(), memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        vkAllocateMemory(device_, &memAlloc, nullptr, &offscreen.memory);
        vkBindImageMemory(device_, offscreen.image, offscreen.memory, 0);

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
        vkCreateImageView(device_, &colorImageView, nullptr, &offscreen.view);

        VkFramebufferCreateInfo fbufCreateInfo{};
        fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbufCreateInfo.renderPass = prefEMapRenderpass_;
        fbufCreateInfo.attachmentCount = 1;
        fbufCreateInfo.pAttachments = &offscreen.view;
        fbufCreateInfo.width = width_;
        fbufCreateInfo.height = height_;
        fbufCreateInfo.layers = 1;
        vkCreateFramebuffer(device_, &fbufCreateInfo, nullptr, &offscreen.framebuffer);

        VkCommandBuffer layoutCmd = pDevHelper_->beginSingleTimeCommands();
        VkImageSubresourceRange subRange{};
        subRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        subRange.baseMipLevel = 0;
        subRange.levelCount = 1;
        subRange.layerCount = 1;
        transitionImageLayout(layoutCmd, subRange, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, offscreen.image);
        endCommandBuffer(device_, layoutCmd, pGraphicsQueue_, pCommandPool_);
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
    VkPushConstantRange pcRange{};
    pcRange.offset = 0;
    pcRange.size = sizeof(PushBlock);
    pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayout descSetLayouts[] = { prefEMapDescriptorSetLayout_ };

    // We can use uniform values to make changes to the shaders without having to create them again, similar to global variables
    // Initialize the pipeline layout with another create info struct
    VkPipelineLayoutCreateInfo pipeLineLayoutCInfo{};
    pipeLineLayoutCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeLineLayoutCInfo.setLayoutCount = 1;
    pipeLineLayoutCInfo.pSetLayouts = descSetLayouts;
    pipeLineLayoutCInfo.pushConstantRangeCount = 1;
    pipeLineLayoutCInfo.pPushConstantRanges = &pcRange;

    if (vkCreatePipelineLayout(device_, &pipeLineLayoutCInfo, nullptr, &(prefEMapPipelineLayout_)) != VK_SUCCESS) {
        std::cout << "nah you buggin" << std::endl;
        std::_Xruntime_error("Failed to create brdfLUT pipeline layout!");
    }

    std::vector<char> prefEMapVertShader = readFile("C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/filterCubeVert.spv");
    std::vector<char> prefEMapFragShader = readFile("C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/prefilteredEnvMapFrag.spv");

    std::cout << "read files" << std::endl;

    VkShaderModule prefEMapVertexShaderModule = createShaderModule(device_, prefEMapVertShader);
    VkShaderModule prefEMapFragmentShaderModule = createShaderModule(device_, prefEMapFragShader);

    // Next struct describes what kind of geometry will be drawn from the verts and if primitive restart should be enabled
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCInfo{};
    inputAssemblyCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCInfo.primitiveRestartEnable = VK_FALSE;

    // Initialize the viewport information struct, a lot of the size information will come from the swap chain extent factor
    VkPipelineViewportStateCreateInfo viewportStateCInfo{};
    viewportStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCInfo.viewportCount = 1;
    viewportStateCInfo.scissorCount = 1;

    // Initialize rasterizer, which takes information from the geometry formed by the vertex shader into fragments to be colored by the fragment shader
    VkPipelineRasterizationStateCreateInfo rasterizerCInfo{};
    rasterizerCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    // Fragments beyond the near and far planes are clamped to those planes, instead of discarding them
    rasterizerCInfo.depthClampEnable = VK_FALSE;
    // If set to true, geometry never passes through the rasterization phase, and disables output to framebuffer
    rasterizerCInfo.rasterizerDiscardEnable = VK_FALSE;
    // Determines how fragments are generated for geometry, using other modes requires enabling a GPU feature
    rasterizerCInfo.polygonMode = VK_POLYGON_MODE_FILL;
    // Linewidth describes thickness of lines in terms of number of fragments 
    rasterizerCInfo.lineWidth = 1.0f;
    // Specify type of culling and and the vertex order for the faces to be considered
    rasterizerCInfo.cullMode = VK_CULL_MODE_NONE;
    rasterizerCInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    // Alter depth values by adding constant or biasing them based on a fragment's slope
    rasterizerCInfo.depthBiasEnable = VK_FALSE;

    // Multisampling information struct
    VkPipelineMultisampleStateCreateInfo multiSamplingCInfo{};
    multiSamplingCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multiSamplingCInfo.sampleShadingEnable = VK_FALSE;
    multiSamplingCInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Depth and stencil testing would go here, but not doing this for the triangle
    VkPipelineDepthStencilStateCreateInfo depthStencilCInfo{};
    depthStencilCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilCInfo.depthTestEnable = VK_FALSE;
    depthStencilCInfo.depthWriteEnable = VK_FALSE;
    depthStencilCInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthStencilCInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilCInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;

    // Color blending - color from fragment shader needs to be combined with color already in the framebuffer
    // If <blendEnable> is set to false, then the color from the fragment shader is passed through to the framebuffer
    // Otherwise, combine with a colorWriteMask to determine the channels that are passed through
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = 0xf;
    colorBlendAttachment.blendEnable = VK_FALSE;

    // Array of structures for all of the framebuffers to set blend constants as blend factors
    VkPipelineColorBlendStateCreateInfo colorBlendingCInfo{};
    colorBlendingCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendingCInfo.logicOpEnable = VK_FALSE;
    colorBlendingCInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendingCInfo.attachmentCount = 1;
    colorBlendingCInfo.pAttachments = &colorBlendAttachment;
    colorBlendingCInfo.blendConstants[0] = 0.0f;
    colorBlendingCInfo.blendConstants[1] = 0.0f;
    colorBlendingCInfo.blendConstants[2] = 0.0f;
    colorBlendingCInfo.blendConstants[3] = 0.0f;

    // Not much can be changed without completely recreating the rendering pipeline, so we fill in a struct with the information
    std::vector<VkDynamicState> dynaStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCInfo{};
    dynamicStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCInfo.dynamicStateCount = static_cast<uint32_t>(dynaStates.size());
    dynamicStateCInfo.pDynamicStates = dynaStates.data();

    std::cout << "pipeline layout created" << std::endl;

    VkPipelineShaderStageCreateInfo prefEMapVertexStageCInfo{};
    prefEMapVertexStageCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    prefEMapVertexStageCInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    prefEMapVertexStageCInfo.module = prefEMapVertexShaderModule;
    prefEMapVertexStageCInfo.pName = "main";

    VkPipelineShaderStageCreateInfo prefEMapFragmentStageCInfo{};
    prefEMapFragmentStageCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    prefEMapFragmentStageCInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    prefEMapFragmentStageCInfo.module = prefEMapFragmentShaderModule;
    prefEMapFragmentStageCInfo.pName = "main";

    VkPipelineShaderStageCreateInfo stages[] = { prefEMapVertexStageCInfo, prefEMapFragmentStageCInfo };

    // Combine the shader stages, fixed-function state, pipeline layout, and render pass to create the graphics pipeline
    // First - populate struct with the information
    VkGraphicsPipelineCreateInfo brdfLUTPipelineCInfo{};
    brdfLUTPipelineCInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    brdfLUTPipelineCInfo.stageCount = 2;
    brdfLUTPipelineCInfo.pStages = stages;

    // Describing the format of the vertex data to be passed to the vertex shader
    VkPipelineVertexInputStateCreateInfo vertexInputCInfo{};
    vertexInputCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescription = MeshHelper::Vertex::getBindingDescription();
    auto attributeDescriptions = MeshHelper::Vertex::getAttributeDescriptions();

    vertexInputCInfo.vertexBindingDescriptionCount = 1;
    vertexInputCInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputCInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());;
    vertexInputCInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    brdfLUTPipelineCInfo.pVertexInputState = &vertexInputCInfo;
    brdfLUTPipelineCInfo.pInputAssemblyState = &inputAssemblyCInfo;
    brdfLUTPipelineCInfo.pViewportState = &viewportStateCInfo;
    brdfLUTPipelineCInfo.pRasterizationState = &rasterizerCInfo;
    brdfLUTPipelineCInfo.pMultisampleState = &multiSamplingCInfo;
    brdfLUTPipelineCInfo.pDepthStencilState = &depthStencilCInfo;
    brdfLUTPipelineCInfo.pColorBlendState = &colorBlendingCInfo;
    brdfLUTPipelineCInfo.pDynamicState = &dynamicStateCInfo;

    brdfLUTPipelineCInfo.layout = prefEMapPipelineLayout_;

    brdfLUTPipelineCInfo.renderPass = prefEMapRenderpass_;
    brdfLUTPipelineCInfo.subpass = 0;

    brdfLUTPipelineCInfo.basePipelineHandle = VK_NULL_HANDLE;

    vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &brdfLUTPipelineCInfo, nullptr, &prefEMapPipeline_);

    std::cout << "pipeline created" << std::endl;
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
void PrefilteredEnvMap::render() {
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

    for (int mip = 0; mip < mipLevels_; mip++) {
        pushBlock.roughness = (float)mip / (float)(mipLevels_ - 1);
        for (int face = 0; face < 6; face++) {
            viewport.width = static_cast<float>(width_ * std::pow(0.5f, mip));
            viewport.height = static_cast<float>(height_ * std::pow(0.5f, mip));
            vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

            vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

            pushBlock.mvp = proj * matrices[face];

            vkCmdPushConstants(cmdBuf, prefEMapPipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pushBlock), &pushBlock);

            vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, prefEMapPipeline_);
            vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, prefEMapPipelineLayout_, 0, 1, &prefEMapDescriptorSet_, 0, NULL);

            pSkybox_->pSkyBoxModel_->genImageRenderSkybox(cmdBuf);

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

    endCommandBuffer(device_, cmdBuf, pGraphicsQueue_, pCommandPool_);
}

void PrefilteredEnvMap::genprefEMap() {
    createprefEMapImage();
    createprefEMapImageView();
    createprefEMapImageSampler();

    createRenderPass();
    createFrameBuffer();

    createprefEMapDescriptors();

    createPipeline();
    render();
}

PrefilteredEnvMap::PrefilteredEnvMap(DeviceHelper* devHelper, VkQueue* graphicsQueue, VkCommandPool* cmdPool, Skybox* pSkybox) {
    this->pDevHelper_ = devHelper;
    this->device_ = devHelper->getDevice();
    this->imageFormat_ = VK_FORMAT_R16G16B16A16_SFLOAT;
    this->width_ = this->height_ = 512;
    this->mipLevels_ = static_cast<uint32_t>(floor(log2(64))) + 1;;
    this->pGraphicsQueue_ = graphicsQueue;
    this->pSkybox_ = pSkybox;
    this->pCommandPool_ = cmdPool;
}