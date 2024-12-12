#include "BRDFLut.h"

// CODE PARTIALLY FROM: https://github.com/SaschaWillems/Vulkan/blob/master/examples/pbrtexture/pbrtexture.cpp
void BRDFLut::render() {
	VkClearValue clearValues[1]{};
    clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };

    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = brdfLUTRenderpass_;
    renderPassBeginInfo.renderArea.extent.width = this->imageTarget_.extents.width_;
    renderPassBeginInfo.renderArea.extent.height = this->imageTarget_.extents.height_;
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
    viewport.width = this->imageTarget_.extents.width_;
    viewport.height = this->imageTarget_.extents.height_;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmdBuf, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent.width = this->imageTarget_.extents.width_;
    scissor.extent.height = this->imageTarget_.extents.height_;
    vkCmdSetScissor(cmdBuf, 0, 1, &scissor);

    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, brdfLutPipeline_->pipeline);
    vkCmdDraw(cmdBuf, 3, 1, 0, 0);
    vkCmdEndRenderPass(cmdBuf);

    pDevHelper_->endSingleTimeCommands(cmdBuf);
}

// CODE PARTIALLY FROM: https://github.com/SaschaWillems/Vulkan/blob/master/examples/pbrtexture/pbrtexture.cpp
void BRDFLut::generateBRDFLUT() {
	// create image
	pDevHelper_->createImage(this->imageTarget_.extents.width_, this->imageTarget_.extents.height_, 1, 1, static_cast<VkImageCreateFlagBits>(0), VK_SAMPLE_COUNT_1_BIT, this->imageTarget_.imageFormat_, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0, this->imageTarget_.image_, this->imageTarget_.imageMemory_);

	// create image view
	VkImageViewCreateInfo brdfLutImageViewCI{};
	brdfLutImageViewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	brdfLutImageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D;
	brdfLutImageViewCI.format = this->imageTarget_.imageFormat_;
	brdfLutImageViewCI.subresourceRange = {};
	brdfLutImageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	brdfLutImageViewCI.subresourceRange.levelCount = 1;
	brdfLutImageViewCI.subresourceRange.layerCount = 1;
	brdfLutImageViewCI.image = this->imageTarget_.image_;

	vkCreateImageView(pDevHelper_->device_, &brdfLutImageViewCI, nullptr, &this->imageTarget_.imageView_);

	// create image sampler
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

	vkCreateSampler(pDevHelper_->device_, &brdfLutImageSamplerCI, nullptr, &this->imageTarget_.imageSampler_);

	// create renderpass
	VkAttachmentDescription brdfLUTattachment{};
	brdfLUTattachment.format = this->imageTarget_.imageFormat_;
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
	renderPassCI.pAttachments = &brdfLUTattachment;
	renderPassCI.subpassCount = 1;
	renderPassCI.pSubpasses = &subpassDescription;
	renderPassCI.dependencyCount = 2;
	renderPassCI.pDependencies = dependencies.data();

	vkCreateRenderPass(pDevHelper_->device_, &renderPassCI, nullptr, &brdfLUTRenderpass_);

	// create framebuffer
	VkFramebufferCreateInfo framebufferCI{};
	framebufferCI.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferCI.renderPass = brdfLUTRenderpass_;
	framebufferCI.attachmentCount = 1;
	framebufferCI.pAttachments = &this->imageTarget_.imageView_;
	framebufferCI.width = this->imageTarget_.extents.width_;
	framebufferCI.height = this->imageTarget_.extents.height_;
	framebufferCI.layers = 1;

	vkCreateFramebuffer(pDevHelper_->device_, &framebufferCI, nullptr, &brdfLUTFrameBuffer_);

	// create descriptors
	VulkanDescriptorLayoutBuilder::BindingStruct binding{};
	binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	binding.stageBits = VK_SHADER_STAGE_FRAGMENT_BIT;

	brdfLUTDescriptorSetLayout_ = new VulkanDescriptorLayoutBuilder(pDevHelper_, 1, &binding);

	VkDescriptorSetAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocateInfo.descriptorPool = pDevHelper_->descPool_;
	allocateInfo.descriptorSetCount = 1;
	allocateInfo.pSetLayouts = &brdfLUTDescriptorSetLayout_->layout;

	vkAllocateDescriptorSets(pDevHelper_->device_, &allocateInfo, &brdfLUTDescriptorSet_);

	// create pipeline
	VulkanPipelineBuilder::VulkanShaderModule vertexShaderModule = VulkanPipelineBuilder::VulkanShaderModule(pDevHelper_->device_, "./shaders/spv/brdfLUTVert.spv");
	VulkanPipelineBuilder::VulkanShaderModule fragmentShaderModule = VulkanPipelineBuilder::VulkanShaderModule(pDevHelper_->device_, "./shaders/spv/brdfLUTFrag.spv");

	std::array<VulkanPipelineBuilder::VulkanShaderModule, 2> shaderStages = { vertexShaderModule, fragmentShaderModule };

	VulkanPipelineBuilder::PipelineBuilderInfo pipelineInfo{};
	pipelineInfo.pDescriptorSetLayouts = &brdfLUTDescriptorSetLayout_->layout;
	pipelineInfo.numSets = 1;
	pipelineInfo.pShaderStages = shaderStages.data();
	pipelineInfo.numStages = shaderStages.size();
	pipelineInfo.pPushConstantRanges = nullptr;
	pipelineInfo.numRanges = 0;
	pipelineInfo.vertexBindingDescriptions = nullptr;
	pipelineInfo.numVertexBindingDescriptions = 0;
	pipelineInfo.vertexAttributeDescriptions = nullptr;
	pipelineInfo.numVertexAttributeDescriptions = 0;

	brdfLutPipeline_ = new VulkanPipelineBuilder(pDevHelper_->device_, pipelineInfo, pDevHelper_);

	brdfLutPipeline_->info.pColorBlendState->attachmentCount = 1;
	brdfLutPipeline_->info.pMultisampleState->rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	brdfLutPipeline_->info.pDepthStencilState->depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	brdfLutPipeline_->info.pDepthStencilState->depthTestEnable = VK_FALSE;
	brdfLutPipeline_->info.pDepthStencilState->back.compareOp = VK_COMPARE_OP_ALWAYS;
	brdfLutPipeline_->info.pRasterizationState->cullMode = VK_CULL_MODE_NONE;

	brdfLutPipeline_->generate(pipelineInfo, brdfLUTRenderpass_);

    render();
}

BRDFLut::BRDFLut(DeviceHelper* devHelper) {
	this->pDevHelper_ = devHelper;
	this->imageTarget_ = VulkanImage(devHelper);
	this->imageTarget_.imageFormat_ = VK_FORMAT_R16G16_SFLOAT;
	this->imageTarget_.extents.width_ = this->imageTarget_.extents.height_ = 512;

    generateBRDFLUT();
	preDelete();
}

void BRDFLut::preDelete() {
	vkDestroyFramebuffer(pDevHelper_->device_, this->brdfLUTFrameBuffer_, nullptr);
	vkDestroyRenderPass(pDevHelper_->device_, this->brdfLUTRenderpass_, nullptr);
	delete brdfLUTDescriptorSetLayout_;
	delete brdfLutPipeline_;
	this->pDevHelper_ = nullptr;
}

BRDFLut::~BRDFLut() {
}