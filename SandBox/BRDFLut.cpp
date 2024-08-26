#include "BRDFLut.h"

void BRDFLut::createBRDFLutImage() {
	pDevHelper_->createImage(width_, height_, mipLevels_, 1 , static_cast<VkImageCreateFlagBits>(0), VK_SAMPLE_COUNT_1_BIT, imageFormat_, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 0, brdfLUTImage_, brdfLUTImageMemory_);
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
	
	vkCreateImageView(pDevHelper_->device_, &brdfLutImageViewCI, nullptr, &brdfLUTImageView_);
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

    vkCreateSampler(pDevHelper_->device_, &brdfLutImageSamplerCI, nullptr, &brdfLUTImageSampler_);
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

	vkCreateDescriptorSetLayout(pDevHelper_->device_, &layoutCInfo, nullptr, &brdfLUTDescriptorSetLayout_);

	std::array<VkDescriptorPoolSize, 1> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[0].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolCInfo{};
	poolCInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolCInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolCInfo.pPoolSizes = poolSizes.data();
	poolCInfo.maxSets = 2;

	if (vkCreateDescriptorPool(pDevHelper_->device_, &poolCInfo, nullptr, &brdfLUTDescriptorPool_) != VK_SUCCESS) {
		std::_Xruntime_error("Failed to create the descriptor pool!");
	}

	VkDescriptorSetAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocateInfo.descriptorPool = brdfLUTDescriptorPool_;
	allocateInfo.descriptorSetCount = 1;
	allocateInfo.pSetLayouts = &brdfLUTDescriptorSetLayout_;

	vkAllocateDescriptorSets(pDevHelper_->device_, &allocateInfo, &brdfLUTDescriptorSet_);
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

	vkCreateFramebuffer(pDevHelper_->device_, &framebufferCI, nullptr, &brdfLUTFrameBuffer_);
}

void BRDFLut::createPipeline() {
    VulkanPipelineBuilder::VulkanShaderModule vertexShaderModule = VulkanPipelineBuilder::VulkanShaderModule(pDevHelper_->device_, "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/spv/brdfLUTVert.spv");
    VulkanPipelineBuilder::VulkanShaderModule fragmentShaderModule = VulkanPipelineBuilder::VulkanShaderModule(pDevHelper_->device_, "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/spv/brdfLUTFrag.spv");

    std::array<VkShaderModule, 2> shaderStages = { vertexShaderModule.module, fragmentShaderModule.module };

    VulkanPipelineBuilder::PipelineBuilderInfo pipelineInfo{};
    pipelineInfo.pDescriptorSetLayouts = &brdfLUTDescriptorSetLayout_;
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
}

// CODE PARTIALLY FROM: https://github.com/SaschaWillems/Vulkan/blob/master/examples/pbrtexture/pbrtexture.cpp
void BRDFLut::render() {
	VkClearValue clearValues[1]{};
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

    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, brdfLutPipeline_->pipeline);
    vkCmdDraw(cmdBuf, 3, 1, 0, 0);
    vkCmdEndRenderPass(cmdBuf);

    pDevHelper_->endSingleTimeCommands(cmdBuf);
}

void BRDFLut::generateBRDFLUT() {
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
	this->imageFormat_ = VK_FORMAT_R16G16_SFLOAT;
	this->width_ = this->height_ = 512;
	this->mipLevels_ = 1;

    generateBRDFLUT();
}

BRDFLut::~BRDFLut() {
	vkDestroySampler(pDevHelper_->device_, this->brdfLUTImageSampler_, nullptr);
	vkDestroyImageView(pDevHelper_->device_, this->brdfLUTImageView_, nullptr);
	vkDestroyImage(pDevHelper_->device_, this->brdfLUTImage_, nullptr);
	vkFreeMemory(pDevHelper_->device_, brdfLUTImageMemory_, nullptr);
	vkDestroyFramebuffer(pDevHelper_->device_, this->brdfLUTFrameBuffer_, nullptr);
	vkDestroyRenderPass(pDevHelper_->device_, this->brdfLUTRenderpass_, nullptr);
	vkDestroyDescriptorSetLayout(pDevHelper_->device_, this->brdfLUTDescriptorSetLayout_, nullptr);
	vkDestroyDescriptorPool(pDevHelper_->device_, this->brdfLUTDescriptorPool_, nullptr);
	delete brdfLutPipeline_;
	this->pDevHelper_ = nullptr;
}