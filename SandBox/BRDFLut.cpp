#include "BRDFLut.h"

void BRDFLut::createBRDFLutImage() {
	pDevHelper_->createImage(width_, height_, mipLevels_, VK_SAMPLE_COUNT_1_BIT, imageFormat_, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0, brdfLUTImage_, brdfLUTImageMemory_);
}

// CODE FROM: https://github.com/SaschaWillems/Vulkan/blob/master/examples/pbrtexture/pbrtexture.cpp
void BRDFLut::createBRDFLutImageView() {
	VkImageViewCreateInfo brdfLutImageViewCI{};
    brdfLutImageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	brdfLutImageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
	brdfLutImageViewCI.format = imageFormat_;
	brdfLutImageViewCI.subresourceRange = {};
	brdfLutImageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	brdfLutImageViewCI.subresourceRange.levelCount = 1;
	brdfLutImageViewCI.subresourceRange.layerCount = 1;
	brdfLutImageViewCI.image = brdfLUTImage_;
	
	vkCreateImageView(device_, &brdfLutImageViewCI, nullptr, &brdfLUTImageView_);
}

// CODE FROM: https://github.com/SaschaWillems/Vulkan/blob/master/examples/pbrtexture/pbrtexture.cpp
void BRDFLut::createBRDFLutImageSampler() {
	VkSamplerCreateInfo brdfLutImageSamplerCI{};
    brdfLutImageSamplerCI.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	brdfLutImageSamplerCI.magFilter = VK_FILTER_LINEAR;
	brdfLutImageSamplerCI.minFilter = VK_FILTER_LINEAR;
	brdfLutImageSamplerCI.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	brdfLutImageSamplerCI.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	brdfLutImageSamplerCI.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	brdfLutImageSamplerCI.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	brdfLutImageSamplerCI.minLod = 0.0f;
	brdfLutImageSamplerCI.maxLod = 1.0f;
	brdfLutImageSamplerCI.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

    vkCreateSampler(device_, &brdfLutImageSamplerCI, nullptr, &brdfLUTImageSampler_);
}

// CODE PARTIALLY FROM: https://github.com/SaschaWillems/Vulkan/blob/master/examples/pbrtexture/pbrtexture.cpp
void BRDFLut::createBRDFLutDescriptors() {
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

	vkCreateDescriptorSetLayout(device_, &layoutCInfo, nullptr, &brdfLUTDescriptorSetLayout_);

	std::array<VkDescriptorPoolSize, 1> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[0].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolCInfo{};
	poolCInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolCInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolCInfo.pPoolSizes = poolSizes.data();
	poolCInfo.maxSets = 2;

	if (vkCreateDescriptorPool(device_, &poolCInfo, nullptr, &brdfLUTDescriptorPool_) != VK_SUCCESS) {
		std::_Xruntime_error("Failed to create the descriptor pool!");
	}

	VkDescriptorSetAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocateInfo.descriptorPool = brdfLUTDescriptorPool_;
	allocateInfo.descriptorSetCount = 1;
	allocateInfo.pSetLayouts = &brdfLUTDescriptorSetLayout_;

	vkAllocateDescriptorSets(device_, &allocateInfo, &brdfLUTDescriptorSet_);
}

void BRDFLut::createRenderPass() {
    VkAttachmentDescription brdfLUTattachment{};
	brdfLUTattachment.format = imageFormat_;
	brdfLUTattachment.samples = VK_SAMPLE_COUNT_1_BIT;
	brdfLUTattachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	brdfLUTattachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	brdfLUTattachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	brdfLUTattachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	brdfLUTattachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	brdfLUTattachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
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
	renderPassCI.pAttachments = &brdfLUTattachment;
	renderPassCI.subpassCount = 1;
	renderPassCI.pSubpasses = &subpassDescription;
	renderPassCI.dependencyCount = 2;
	renderPassCI.pDependencies = dependencies.data();

	vkCreateRenderPass(device_, &renderPassCI, nullptr, &brdfLUTRenderpass_);
}
void BRDFLut::createFrameBuffer() {
	VkFramebufferCreateInfo framebufferCI{};
	framebufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferCI.renderPass = brdfLUTRenderpass_;
	framebufferCI.attachmentCount = 1;
	framebufferCI.pAttachments = &brdfLUTImageView_;
	framebufferCI.width = width_;
	framebufferCI.height = height_;
	framebufferCI.layers = 1;

	vkCreateFramebuffer(device_, &framebufferCI, nullptr, &brdfLUTFrameBuffer_);
}

// In order to pass the binary code to the graphics pipeline, we need to create a VkShaderModule object to wrap it with
VkShaderModule createShaderModule(VkDevice dev, const std::vector<char>& binary) {
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

void BRDFLut::createPipeline() {
    VkDescriptorSetLayout descSetLayouts[] = { brdfLUTDescriptorSetLayout_ };

    // We can use uniform values to make changes to the shaders without having to create them again, similar to global variables
    // Initialize the pipeline layout with another create info struct
    VkPipelineLayoutCreateInfo pipeLineLayoutCInfo{};
    pipeLineLayoutCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeLineLayoutCInfo.setLayoutCount = 1;
    pipeLineLayoutCInfo.pSetLayouts = descSetLayouts;

    if (vkCreatePipelineLayout(device_, &pipeLineLayoutCInfo, nullptr, &(brdfLUTPipelineLayout_)) != VK_SUCCESS) {
        std::cout << "nah you buggin" << std::endl;
        std::_Xruntime_error("Failed to create brdfLUT pipeline layout!");
    }

    std::vector<char> brdfVertShader = pDevHelper_->readFile("C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/spv/brdfLUTVert.spv");
    std::vector<char> brdfFragShader = pDevHelper_->readFile("C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/spv/brdfLUTFrag.spv");

    std::cout << "read files" << std::endl;

    VkShaderModule brdfVertexShaderModule = createShaderModule(device_, brdfVertShader);
    VkShaderModule brdfFragmentShaderModule = createShaderModule(device_, brdfFragShader);

    // Describing the format of the vertex data to be passed to the vertex shader
    VkPipelineVertexInputStateCreateInfo vertexInputCInfo{};
    vertexInputCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    vertexInputCInfo.vertexBindingDescriptionCount = 0;

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

    VkPipelineShaderStageCreateInfo brdfVertexStageCInfo{};
    brdfVertexStageCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    brdfVertexStageCInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    brdfVertexStageCInfo.module = brdfVertexShaderModule;
    brdfVertexStageCInfo.pName = "main";

    VkPipelineShaderStageCreateInfo brdfFragmentStageCInfo{};
    brdfFragmentStageCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    brdfFragmentStageCInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    brdfFragmentStageCInfo.module = brdfFragmentShaderModule;
    brdfFragmentStageCInfo.pName = "main";

    VkPipelineShaderStageCreateInfo stages[] = { brdfVertexStageCInfo, brdfFragmentStageCInfo };

    // Combine the shader stages, fixed-function state, pipeline layout, and render pass to create the graphics pipeline
    // First - populate struct with the information
    VkGraphicsPipelineCreateInfo brdfLUTPipelineCInfo{};
    brdfLUTPipelineCInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    brdfLUTPipelineCInfo.stageCount = 2;
    brdfLUTPipelineCInfo.pStages = stages;

    brdfLUTPipelineCInfo.pVertexInputState = &vertexInputCInfo;
    brdfLUTPipelineCInfo.pInputAssemblyState = &inputAssemblyCInfo;
    brdfLUTPipelineCInfo.pViewportState = &viewportStateCInfo;
    brdfLUTPipelineCInfo.pRasterizationState = &rasterizerCInfo;
    brdfLUTPipelineCInfo.pMultisampleState = &multiSamplingCInfo;
    brdfLUTPipelineCInfo.pDepthStencilState = &depthStencilCInfo;
    brdfLUTPipelineCInfo.pColorBlendState = &colorBlendingCInfo;
    brdfLUTPipelineCInfo.pDynamicState = &dynamicStateCInfo;

    brdfLUTPipelineCInfo.layout = brdfLUTPipelineLayout_;

    brdfLUTPipelineCInfo.renderPass = brdfLUTRenderpass_;
    brdfLUTPipelineCInfo.subpass = 0;

    brdfLUTPipelineCInfo.basePipelineHandle = VK_NULL_HANDLE;

    vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &brdfLUTPipelineCInfo, nullptr, &brdfLUTPipeline_);

    std::cout << "pipeline created" << std::endl;
}

// CODE PARTIALLY FROM: https://github.com/SaschaWillems/Vulkan/blob/master/examples/pbrtexture/pbrtexture.cpp
void BRDFLut::render() {
    VkClearValue clearValues[1];
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = brdfLUTRenderpass_;
    renderPassBeginInfo.renderArea.extent.width = width_;
    renderPassBeginInfo.renderArea.extent.height = height_;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = clearValues;
    renderPassBeginInfo.framebuffer = brdfLUTFrameBuffer_;

    VkCommandBuffer cmdBuf = pDevHelper_->beginSingleTimeCommands();

    VkCommandBufferBeginInfo CBBeginInfo{};
    CBBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CBBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = width_;
    viewport.height = height_;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent.width = width_;
    scissor.extent.height = height_;
    vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, brdfLUTPipeline_);
    vkCmdDraw(cmdBuf, 3, 1, 0, 0);
    vkCmdEndRenderPass(cmdBuf);

    pDevHelper_->endSingleTimeCommands(cmdBuf);
}

void BRDFLut::genBRDFLUT() {
	createBRDFLutImage();
	createBRDFLutImageView();
	createBRDFLutImageSampler();

	createRenderPass();
	createFrameBuffer();
	createBRDFLutDescriptors();
    createPipeline();

    render();
}

BRDFLut::BRDFLut(DeviceHelper* devHelper) {
	this->pDevHelper_ = devHelper;
	this->device_ = devHelper->getDevice();
	this->imageFormat_ = VK_FORMAT_R16G16_SFLOAT;
	this->width_ = this->height_ = 512;
	this->mipLevels_ = 1;
}