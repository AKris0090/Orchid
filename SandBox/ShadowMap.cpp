#include "ShadowMap.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS

uint32_t ShadowMap::findMemoryType(VkPhysicalDevice gpu_, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(gpu_, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	std::_Xruntime_error("Failed to find a suitable memory type!");
}

void ShadowMap::findDepthFormat(VkPhysicalDevice GPU_) {
	imageFormat_ = findSupportedFormat(GPU_, { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkFormat ShadowMap::findSupportedFormat(VkPhysicalDevice GPU_, const std::vector<VkFormat>& potentialFormats, VkImageTiling tiling, VkFormatFeatureFlags features) {
	for (VkFormat format : potentialFormats) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(GPU_, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	std::_Xruntime_error("Failed to find a supported format!");
}

// CODE PARTIALLY FROM: https://github.com/SaschaWillems/Vulkan/blob/master/examples/shadowmapping/shadowmapping.cpp
void ShadowMap::createRenderPass() {
	VkAttachmentDescription attachmentDescription{};
	attachmentDescription.format = imageFormat_;
	attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;							// Clear depth at beginning of the render pass
	attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;						// We will read from depth, so it's important to store the depth attachment results
	attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;					// We don't care about initial layout of the attachment
	attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;// Attachment will be transitioned to shader read at render pass end

	VkAttachmentReference depthReference = {};
	depthReference.attachment = 0;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;			// Attachment will be used as depth/stencil during render pass

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 0;													// No color attachments
	subpass.pDepthStencilAttachment = &depthReference;									// Reference to our depth attachment

	// Use subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderPassCreateInfo{};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &attachmentDescription;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpass;
	renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	renderPassCreateInfo.pDependencies = dependencies.data();

	vkCreateRenderPass(device_, &renderPassCreateInfo, nullptr, &sMRenderpass_);
}

// CODE PARTIALLY FROM: https://github.com/SaschaWillems/Vulkan/blob/master/examples/shadowmapping/shadowmapping.cpp
void ShadowMap::createFrameBuffer() { 
	VkImageCreateInfo image{};
	image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image.imageType = VK_IMAGE_TYPE_2D;
	image.extent.width = width_;
	image.extent.height = height_;
	image.extent.depth = 1;
	image.mipLevels = 1;
	image.arrayLayers = 1;
	image.samples = VK_SAMPLE_COUNT_1_BIT;
	image.tiling = VK_IMAGE_TILING_OPTIMAL;
	image.format = imageFormat_;																// Depth stencil attachment
	image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;		// We will sample directly from the depth attachment for the shadow mapping
	vkCreateImage(device_, &image, nullptr, &offscreen.image);

	VkMemoryAllocateInfo memAlloc{};
	memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(device_, offscreen.image, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	memAlloc.memoryTypeIndex = findMemoryType(pDevHelper_->getPhysicalDevice(), memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vkAllocateMemory(device_, &memAlloc, nullptr, &offscreen.memory);
	vkBindImageMemory(device_, offscreen.image, offscreen.memory, 0);

	VkImageViewCreateInfo depthStencilView{};
	depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthStencilView.format = imageFormat_;
	depthStencilView.subresourceRange = {};
	depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	depthStencilView.subresourceRange.baseMipLevel = 0;
	depthStencilView.subresourceRange.levelCount = 1;
	depthStencilView.subresourceRange.baseArrayLayer = 0;
	depthStencilView.subresourceRange.layerCount = 1;
	depthStencilView.image = offscreen.image;
	vkCreateImageView(device_, &depthStencilView, nullptr, &sMImageView_);

	// Create sampler to sample from to depth attachment
	// Used to sample in the fragment shader for shadowed rendering
	VkFilter shadowmap_filter = VK_FILTER_LINEAR;
	VkSamplerCreateInfo sampler{};
	sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler.magFilter = shadowmap_filter;
	sampler.minFilter = shadowmap_filter;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.addressModeV = sampler.addressModeU;
	sampler.addressModeW = sampler.addressModeU;
	sampler.mipLodBias = 0.0f;
	sampler.maxAnisotropy = 1.0f;
	sampler.minLod = 0.0f;
	sampler.maxLod = 1.0f;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	vkCreateSampler(device_, &sampler, nullptr, &sMImageSampler_);


	sMImageInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
	sMImageInfo.imageView = sMImageView_;
	sMImageInfo.sampler = sMImageSampler_;

	createRenderPass();

	// Create frame buffer
	VkFramebufferCreateInfo fbufCreateInfo{};
	fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbufCreateInfo.renderPass = sMRenderpass_;
	fbufCreateInfo.attachmentCount = 1;
	fbufCreateInfo.pAttachments = &sMImageView_;
	fbufCreateInfo.width = width_;
	fbufCreateInfo.height = height_;
	fbufCreateInfo.layers = 1;

	vkCreateFramebuffer(device_, &fbufCreateInfo, nullptr, &sMFrameBuffer_);
}

// CODE PARTIALLY FROM: https://github.com/SaschaWillems/Vulkan/blob/master/examples/shadowmapping/shadowmapping.cpp
void ShadowMap::createSMDescriptors() {
	VkDescriptorSetLayoutBinding samplerLayoutBindingDepth{};
	samplerLayoutBindingDepth.binding = 0;
	samplerLayoutBindingDepth.descriptorCount = 1;
	samplerLayoutBindingDepth.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBindingDepth.pImmutableSamplers = nullptr;
	samplerLayoutBindingDepth.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutCreateInfo layoutCInfo{};
	layoutCInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCInfo.bindingCount = 1;
	layoutCInfo.pBindings = &(samplerLayoutBindingDepth);

	vkCreateDescriptorSetLayout(device_, &layoutCInfo, nullptr, &sMDescriptorSetLayout_);

	std::array<VkDescriptorPoolSize, 1> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[0].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolCInfo{};
	poolCInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolCInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolCInfo.pPoolSizes = poolSizes.data();
	poolCInfo.maxSets = 2;

	if (vkCreateDescriptorPool(device_, &poolCInfo, nullptr, &sMDescriptorPool_) != VK_SUCCESS) {
		std::_Xruntime_error("Failed to create the descriptor pool!");
	}

	VkDescriptorSetAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocateInfo.descriptorPool = sMDescriptorPool_;
	allocateInfo.descriptorSetCount = 1;
	allocateInfo.pSetLayouts = &sMDescriptorSetLayout_;

	vkAllocateDescriptorSets(device_, &allocateInfo, &sMDescriptorSet_);

	// WRITE SET
	VkWriteDescriptorSet sMDescriptorWriteSet{};
	sMDescriptorWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	sMDescriptorWriteSet.dstSet = sMDescriptorSet_;
	sMDescriptorWriteSet.dstBinding = 0;
	sMDescriptorWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sMDescriptorWriteSet.descriptorCount = 1;
	sMDescriptorWriteSet.pImageInfo = &sMImageInfo;

	std::vector<VkWriteDescriptorSet> descriptorWriteSets = { sMDescriptorWriteSet };

	vkUpdateDescriptorSets(pDevHelper_->getDevice(), static_cast<uint32_t>(descriptorWriteSets.size()), descriptorWriteSets.data(), 0, nullptr);
}

VkShaderModule ShadowMap::createShaderModule(VkDevice dev, const std::vector<char>& binary) {
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

std::vector<char> ShadowMap::readFile(const std::string& filename) {
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

void ShadowMap::createPipeline() {
    VkPushConstantRange pcRange{};
    pcRange.offset = 0;
    pcRange.size = sizeof(GLTFObj::depthMVModel);
    pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayout descSetLayouts[] = { sMDescriptorSetLayout_ };

    VkPipelineLayoutCreateInfo pipeLineLayoutCInfo{};
    pipeLineLayoutCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeLineLayoutCInfo.setLayoutCount = 1;
    pipeLineLayoutCInfo.pSetLayouts = descSetLayouts;
    pipeLineLayoutCInfo.pushConstantRangeCount = 1;
    pipeLineLayoutCInfo.pPushConstantRanges = &pcRange;

    if (vkCreatePipelineLayout(device_, &pipeLineLayoutCInfo, nullptr, &(sMPipelineLayout_)) != VK_SUCCESS) {
        std::cout << "nah you buggin" << std::endl;
        std::_Xruntime_error("Failed to create brdfLUT pipeline layout!");
    }

    std::vector<char> sMVertShader = readFile("C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/shadowMap.spv");
	std::vector<char> sMFragShader = readFile("C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/shadowMapFrag.spv");

	//std::cout << "read files" << std::endl;

    VkShaderModule sMVertexShaderModule = createShaderModule(device_, sMVertShader);
	VkShaderModule sMFragShaderModule = createShaderModule(device_, sMFragShader);

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCInfo{};
    inputAssemblyCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCInfo.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportStateCInfo{};
    viewportStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCInfo.viewportCount = 1;
    viewportStateCInfo.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizerCInfo{};
	rasterizerCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerCInfo.pNext = nullptr;
	rasterizerCInfo.flags = 0;
	rasterizerCInfo.depthClampEnable = VK_FALSE;
	rasterizerCInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizerCInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizerCInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizerCInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizerCInfo.depthBiasEnable = VK_FALSE;
	rasterizerCInfo.depthBiasConstantFactor = 0.0f;
	rasterizerCInfo.depthBiasClamp = 0.0f;
	rasterizerCInfo.depthBiasSlopeFactor = 0.0f;
	rasterizerCInfo.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multiSamplingCInfo{};
    multiSamplingCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multiSamplingCInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencilCInfo{};
	depthStencilCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilCInfo.pNext = nullptr;
	depthStencilCInfo.flags = 0;
	depthStencilCInfo.depthTestEnable = VK_TRUE;
	depthStencilCInfo.depthWriteEnable = VK_TRUE;
	depthStencilCInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilCInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilCInfo.stencilTestEnable = VK_FALSE;
	depthStencilCInfo.front = {};
	depthStencilCInfo.back = {};
	depthStencilCInfo.minDepthBounds = 0.0f;
	depthStencilCInfo.maxDepthBounds = 1.0f;

    VkPipelineColorBlendStateCreateInfo colorBlendingCInfo{};
    colorBlendingCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendingCInfo.attachmentCount = 0;

    std::vector<VkDynamicState> dynaStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCInfo{};
    dynamicStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCInfo.dynamicStateCount = static_cast<uint32_t>(dynaStates.size());
    dynamicStateCInfo.pDynamicStates = dynaStates.data();

    //std::cout << "pipeline layout created" << std::endl;

    VkPipelineShaderStageCreateInfo sMVertexStageCInfo{};
    sMVertexStageCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    sMVertexStageCInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    sMVertexStageCInfo.module = sMVertexShaderModule;
    sMVertexStageCInfo.pName = "main";

	VkPipelineShaderStageCreateInfo sMFragmentStageCInfo{};
	sMFragmentStageCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	sMFragmentStageCInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	sMFragmentStageCInfo.module = sMFragShaderModule;
	sMFragmentStageCInfo.pName = "main";

    VkPipelineShaderStageCreateInfo stages[] = { sMVertexStageCInfo, sMFragmentStageCInfo };

    VkGraphicsPipelineCreateInfo shadowMapPipelineCInfo{};
    shadowMapPipelineCInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    shadowMapPipelineCInfo.stageCount = 2;
    shadowMapPipelineCInfo.pStages = stages;

    // Describing the format of the vertex data to be passed to the vertex shader
    VkPipelineVertexInputStateCreateInfo vertexInputCInfo{};
    vertexInputCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescription = MeshHelper::Vertex::getBindingDescription();
    auto attributeDescriptions = MeshHelper::Vertex::getPositionAttributeDescription();

    vertexInputCInfo.vertexBindingDescriptionCount = 1;
    vertexInputCInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputCInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());;
    vertexInputCInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    shadowMapPipelineCInfo.pVertexInputState = &vertexInputCInfo;
    shadowMapPipelineCInfo.pInputAssemblyState = &inputAssemblyCInfo;
    shadowMapPipelineCInfo.pViewportState = &viewportStateCInfo;
    shadowMapPipelineCInfo.pRasterizationState = &rasterizerCInfo;
    shadowMapPipelineCInfo.pMultisampleState = &multiSamplingCInfo;
    shadowMapPipelineCInfo.pDepthStencilState = &depthStencilCInfo;
    shadowMapPipelineCInfo.pColorBlendState = &colorBlendingCInfo;
    shadowMapPipelineCInfo.pDynamicState = &dynamicStateCInfo;

    shadowMapPipelineCInfo.layout = sMPipelineLayout_;

    shadowMapPipelineCInfo.renderPass = sMRenderpass_;
    shadowMapPipelineCInfo.subpass = 0;

    shadowMapPipelineCInfo.basePipelineHandle = VK_NULL_HANDLE;

    vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &shadowMapPipelineCInfo, nullptr, &sMPipeline_);

    //std::cout << "pipeline created" << std::endl;
}


void ShadowMap::endCommandBuffer(VkDevice device_, VkCommandBuffer cmdBuff, VkQueue* pGraphicsQueue_, VkCommandPool* pCommandPool_) {
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

    vkDeviceWaitIdle(device_);

    vkFreeCommandBuffers(device_, *(pCommandPool_), 1, &cmdBuff);
}

// CODE PARTIALLY FROM: https://github.com/SaschaWillems/Vulkan/blob/master/examples/pbrtexture/pbrtexture.cpp
void ShadowMap::render(VkCommandBuffer cmdBuf) {
    VkClearValue clearValues[1];
    clearValues[0].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = sMRenderpass_;
    renderPassBeginInfo.renderArea.extent.width = width_;
    renderPassBeginInfo.renderArea.extent.height = height_;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = clearValues;
    renderPassBeginInfo.framebuffer = sMFrameBuffer_;

	vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
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

    vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, sMPipeline_);
    vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, sMPipelineLayout_, 0, 1, &sMDescriptorSet_, 0, NULL);

	for (int i = 0; i < numModels_; i++) {
		pModels_[i]->renderBasic(cmdBuf, sMPipelineLayout_, depthPushBlock.mvp);
	}

    vkCmdEndRenderPass(cmdBuf);
}

void ShadowMap::updateUniBuffers() {
	// Matrix from light's point of view
	glm::vec3 pos = glm::vec3(*lightPos);
	float near_plane = zNear, far_plane = zFar;
	glm::mat4 lightProjection = glm::perspective(glm::radians(45.0f), 1.0f, near_plane, far_plane);
	lightProjection[1][1] *= -1;
	glm::mat4 depthViewMatrix = glm::lookAt(pos, glm::vec3(0.0f), glm::vec3(0, 1, 0));

	depthPushBlock.mvp = lightProjection * depthViewMatrix;
}

void ShadowMap::genShadowMap() {
	width_ = 8192;
	height_ = 8192;
	zNear = 10.0f;
	zFar = 100.0f;
	imageFormat_ = VK_FORMAT_D16_UNORM;
	updateUniBuffers();

	createFrameBuffer(); // includes createRenderPass. CreateRenderPass includes creating image, image view, and image sampler

	createSMDescriptors();

	createPipeline();
}

ShadowMap::ShadowMap(DeviceHelper* devHelper, VkQueue* graphicsQueue, VkCommandPool* cmdPool, glm::vec4* lPos, std::vector<GLTFObj*> pModels_, uint32_t numModels_) {
	this->pDevHelper_ = devHelper;
	this->device_ = devHelper->getDevice();
	this->pGraphicsQueue_ = graphicsQueue;
	this->pCommandPool_ = cmdPool;
	this->lightPos = lPos;
	this->numModels_ = numModels_;
	this->pModels_ = pModels_;
	this->imageFormat_ = VK_FORMAT_D16_UNORM;
}