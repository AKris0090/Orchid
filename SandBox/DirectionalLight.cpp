#include "DirectionalLight.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

uint32_t DirectionalLight::findMemoryType(VkPhysicalDevice gpu_, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(gpu_, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	std::_Xruntime_error("Failed to find a suitable memory type!");
}

VkFormat DirectionalLight::findSupportedFormat(VkPhysicalDevice GPU_) {
	std::vector<VkFormat> potentialFormats = { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D16_UNORM_S8_UINT, VK_FORMAT_D16_UNORM };
	for (VkFormat& format : potentialFormats) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(GPU_, format, &props);

		if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			if (!(props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)) {
				continue;
			}
			return format;
		}
	}

	std::_Xruntime_error("Failed to find a supported format!");
}

// CODE PARTIALLY FROM: https://github.com/SaschaWillems/Vulkan/blob/master/examples/shadowmapping/shadowmapping.cpp
void DirectionalLight::createRenderPass() {
	VkAttachmentDescription attachmentDescription{};
	attachmentDescription.format = imageFormat_;
	attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

	VkAttachmentReference depthReference = {};
	depthReference.attachment = 0;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 0;
	subpass.pDepthStencilAttachment = &depthReference;

	// Use subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 2> dependencies{};

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

	vkCreateRenderPass(pDevHelper_->device_, &renderPassCreateInfo, nullptr, &sMRenderpass_);
}

// CODE PARTIALLY FROM: https://github.com/SaschaWillems/Vulkan/blob/master/examples/shadowmapping/shadowmapping.cpp
void DirectionalLight::createFrameBuffer(int framesInFlight) {
	VkImageCreateInfo image{};
	image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image.imageType = VK_IMAGE_TYPE_2D;
	image.extent.width = width_;
	image.extent.height = height_;
	image.extent.depth = 1;
	image.mipLevels = 1;
	image.arrayLayers = SHADOW_MAP_CASCADE_COUNT;
	image.samples = VK_SAMPLE_COUNT_1_BIT;
	image.tiling = VK_IMAGE_TILING_OPTIMAL;
	image.format = imageFormat_;																// Depth stencil attachment
	image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;		// We will sample directly from the depth attachment for the shadow mapping
	vkCreateImage(pDevHelper_->device_, &image, nullptr, &offscreen.image);

	VkMemoryAllocateInfo memAlloc{};
	memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(pDevHelper_->device_, offscreen.image, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	memAlloc.memoryTypeIndex = findMemoryType(pDevHelper_->gpu_, memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	vkAllocateMemory(pDevHelper_->device_, &memAlloc, nullptr, &offscreen.memory);
	vkBindImageMemory(pDevHelper_->device_, offscreen.image, offscreen.memory, 0);

	VkImageViewCreateInfo depthStencilView{};
	depthStencilView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	depthStencilView.format = imageFormat_;
	depthStencilView.subresourceRange = {};
	depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	depthStencilView.subresourceRange.baseMipLevel = 0;
	depthStencilView.subresourceRange.levelCount = 1;
	depthStencilView.subresourceRange.baseArrayLayer = 0;
	depthStencilView.subresourceRange.layerCount = SHADOW_MAP_CASCADE_COUNT;
	depthStencilView.image = offscreen.image;
	vkCreateImageView(pDevHelper_->device_, &depthStencilView, nullptr, &sMImageView_);

	createRenderPass();

	for (int j = 0; j < framesInFlight; j++) {
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			viewInfo.format = imageFormat_;
			viewInfo.subresourceRange = {};
			viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			viewInfo.subresourceRange.baseMipLevel = 0;
			viewInfo.subresourceRange.levelCount = 1;
			viewInfo.subresourceRange.baseArrayLayer = i;
			viewInfo.subresourceRange.layerCount = 1;
			viewInfo.image = offscreen.image;
			vkCreateImageView(pDevHelper_->device_, &viewInfo, nullptr, &cascades[j][i].imageView);

			VkFramebufferCreateInfo fbufCreateInfo{};
			fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			fbufCreateInfo.renderPass = sMRenderpass_;
			fbufCreateInfo.attachmentCount = 1;
			fbufCreateInfo.pAttachments = &cascades[j][i].imageView;
			fbufCreateInfo.width = width_;
			fbufCreateInfo.height = height_;
			fbufCreateInfo.layers = 1;
			vkCreateFramebuffer(pDevHelper_->device_, &fbufCreateInfo, nullptr, &cascades[j][i].frameBuffer);
		}
	}

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
	vkCreateSampler(pDevHelper_->device_, &sampler, nullptr, &sMImageSampler_);
}

// CODE PARTIALLY FROM: https://github.com/SaschaWillems/Vulkan/blob/master/examples/shadowmapping/shadowmapping.cpp
void DirectionalLight::createSMDescriptors(FPSCam* camera, int framesInFlight) {
	VkDeviceSize bufferSize = sizeof(UBO);

	uniformBuffer.resize(framesInFlight);
	uniformMemory.resize(framesInFlight);
	mappedBuffer.resize(framesInFlight);

	for (int i = 0; i < framesInFlight; i++) {
		pDevHelper_->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffer[i], uniformMemory[i]);
		VkResult res1 = vkMapMemory(pDevHelper_->device_, uniformMemory[i], 0, VK_WHOLE_SIZE, 0, &mappedBuffer[i]);
	}
	
	updateUniBuffers(camera, 0);

	std::vector<VulkanDescriptorLayoutBuilder::BindingStruct> bindings;
	bindings.resize(1);

	bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	bindings[0].stageBits = VK_SHADER_STAGE_VERTEX_BIT;

	cascadeSetLayout = new VulkanDescriptorLayoutBuilder(pDevHelper_, bindings);

	std::vector<VkDescriptorPoolSize> poolSizes{};
	poolSizes.resize(framesInFlight);
	for (int i = 0; i < framesInFlight; i++) {
		poolSizes[i].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[i].descriptorCount = SHADOW_MAP_CASCADE_COUNT;
	}

	VkDescriptorPoolCreateInfo poolCInfo{};
	poolCInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolCInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolCInfo.pPoolSizes = poolSizes.data();
	poolCInfo.maxSets = SHADOW_MAP_CASCADE_COUNT * framesInFlight;

	if (vkCreateDescriptorPool(pDevHelper_->device_, &poolCInfo, nullptr, &sMDescriptorPool_) != VK_SUCCESS) {
		std::_Xruntime_error("Failed to create the descriptor pool!");
	}

	VkDescriptorSetAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocateInfo.descriptorPool = sMDescriptorPool_;
	allocateInfo.descriptorSetCount = 1;
	allocateInfo.pSetLayouts = &cascadeSetLayout->layout;

	for (int j = 0; j < framesInFlight; j++) {
		for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
			VkResult res2 = vkAllocateDescriptorSets(pDevHelper_->device_, &allocateInfo, &cascades[j][i].descriptorSet);

			VkDescriptorBufferInfo descriptorBufferInfo{};
			descriptorBufferInfo.buffer = uniformBuffer[j];
			descriptorBufferInfo.offset = 0;
			descriptorBufferInfo.range = sizeof(UBO);

			VkWriteDescriptorSet bufferWriteSet{};
			bufferWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			bufferWriteSet.dstSet = cascades[j][i].descriptorSet;
			bufferWriteSet.dstBinding = 0;
			bufferWriteSet.dstArrayElement = 0;
			bufferWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			bufferWriteSet.descriptorCount = 1;
			bufferWriteSet.pBufferInfo = &descriptorBufferInfo;

			std::array<VkWriteDescriptorSet, 1> descriptors = { bufferWriteSet };

			vkUpdateDescriptorSets(pDevHelper_->device_, 1, descriptors.data(), 0, NULL);
		}
	}
}

VkShaderModule DirectionalLight::createShaderModule(VkDevice dev, const std::vector<char>& binary) {
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

std::vector<char> DirectionalLight::readFile(const std::string& filename) {
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

void DirectionalLight::createPipeline(VulkanDescriptorLayoutBuilder* modelMatrixDescriptorSet) {
	VulkanPipelineBuilder::VulkanShaderModule vertexShaderModule = VulkanPipelineBuilder::VulkanShaderModule(pDevHelper_->device_, "./shaders/spv/shadowMap.spv");

	std::array<VulkanPipelineBuilder::VulkanShaderModule, 1> shaderStages = { vertexShaderModule };

	VkPushConstantRange pcRange{};
	pcRange.offset = 0;
	pcRange.size = sizeof(int);
	pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	std::array<VkDescriptorSetLayout, 2> sets = { cascadeSetLayout->layout, modelMatrixDescriptorSet->layout };

	auto bindings = Vertex::getBindingDescription();
	auto attributes = Vertex::getPositionAttributeDescription();

	VulkanPipelineBuilder::PipelineBuilderInfo pipelineInfo{};
	pipelineInfo.pDescriptorSetLayouts = sets.data();
	pipelineInfo.numSets = sets.size();
	pipelineInfo.pShaderStages = shaderStages.data();
	pipelineInfo.numStages = shaderStages.size();
	pipelineInfo.pPushConstantRanges = &pcRange;
	pipelineInfo.numRanges = 1;
	pipelineInfo.vertexBindingDescriptions = &bindings;
	pipelineInfo.numVertexBindingDescriptions = 1;
	pipelineInfo.vertexAttributeDescriptions = attributes.data();
	pipelineInfo.numVertexAttributeDescriptions = static_cast<int>(attributes.size());

	sMPipeline_ = new VulkanPipelineBuilder(pDevHelper_->device_, pipelineInfo, pDevHelper_);

	sMPipeline_->info.pRasterizationState->depthClampEnable = VK_TRUE;
	sMPipeline_->info.pRasterizationState->cullMode = VK_CULL_MODE_FRONT_BIT;

	sMPipeline_->info.pMultisampleState->rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineDepthStencilStateCreateInfo* depthStencilCInfo = new VkPipelineDepthStencilStateCreateInfo();
	depthStencilCInfo->sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilCInfo->pNext = nullptr;
	depthStencilCInfo->flags = 0;
	depthStencilCInfo->depthTestEnable = VK_TRUE;
	depthStencilCInfo->depthWriteEnable = VK_TRUE;
	depthStencilCInfo->depthCompareOp = VK_COMPARE_OP_LESS;

	delete sMPipeline_->info.pDepthStencilState;
	sMPipeline_->info.pDepthStencilState = depthStencilCInfo;

	sMPipeline_->info.pColorBlendState->attachmentCount = 0;
	sMPipeline_->info.pColorBlendState->pAttachments = nullptr;

	std::vector<VkDynamicState> dynaStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
			VK_DYNAMIC_STATE_CULL_MODE
	};

	sMPipeline_->info.pDynamicState->dynamicStateCount = static_cast<uint32_t>(dynaStates.size());
	sMPipeline_->info.pDynamicState->pDynamicStates = dynaStates.data();

	sMPipeline_->generate(pipelineInfo, sMRenderpass_);
}

// CODE PARTIALLY FROM: https://github.com/SaschaWillems/Vulkan/blob/master/examples/pbrtexture/pbrtexture.cpp
DirectionalLight::PostRenderPacket DirectionalLight::render(VkCommandBuffer cmdBuf, uint32_t cascadeIndex, int currentFrame) {
	VkClearValue clearValues[1]{};
    clearValues[0].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = sMRenderpass_;
    renderPassBeginInfo.renderArea.extent.width = width_;
    renderPassBeginInfo.renderArea.extent.height = height_;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = clearValues;
	renderPassBeginInfo.framebuffer = cascades[currentFrame][cascadeIndex].frameBuffer;

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

	vkCmdBeginRenderPass(cmdBuf, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

	return { renderPassBeginInfo, sMPipeline_->pipeline, sMPipeline_->layout, cmdBuf };
}

void DirectionalLight::updateUniBuffers(FPSCam* camera, int currentFrame) {
	float nearClip = camera->getNearPlane();
	float farClip = camera->getFarPlane();
	float clipRange = farClip - nearClip;

	float minZ = nearClip;
	float maxZ = nearClip + clipRange;

	float range = maxZ - minZ;
	float ratio = maxZ / minZ;

	// Calculate split depths based on view camera frustum
	// Based on method presented in https://developer.nvidia.com/gpugems/GPUGems3/gpugems3_ch10.html
	for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
		float p = (i + 1) / static_cast<float>(SHADOW_MAP_CASCADE_COUNT);
		float log = minZ * std::pow(ratio, p);
		float uniform = minZ + range * p;
		float d = cascadeSplitLambda * (log - uniform) + uniform;
		shadowCascadeLevels[i] = (d - nearClip) / clipRange;
	}

	// Calculate orthographic projection matrix for each cascade
	float lastSplitDist = 0.0;
	for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
		float splitDist = shadowCascadeLevels[i];

		//glm::vec3 frustumCorners[8] = {
		//	glm::vec3(-1.0f,  1.0f, 0.0f),
		//	glm::vec3(1.0f,  1.0f, 0.0f),
		//	glm::vec3(1.0f, -1.0f, 0.0f),
		//	glm::vec3(-1.0f, -1.0f, 0.0f),
		//	glm::vec3(-1.0f,  1.0f,  1.0f),
		//	glm::vec3(1.0f,  1.0f,  1.0f),
		//	glm::vec3(1.0f, -1.0f,  1.0f),
		//	glm::vec3(-1.0f, -1.0f,  1.0f),
		//};

		glm::vec3 frustumCorners[8] = {
		glm::vec3(-1.0f,  1.0f, 0.0f),
		glm::vec3(1.0f,  1.0f, 0.0f),
		glm::vec3(1.0f, -1.0f, 0.0f),
		glm::vec3(-1.0f, -1.0f, 0.0f),
		glm::vec3(-1.0f,  1.0f,  1.0f),
		glm::vec3(1.0f,  1.0f,  1.0f),
		glm::vec3(1.0f, -1.0f,  1.0f),
		glm::vec3(-1.0f, -1.0f,  1.0f),
		};

		// Project frustum corners into world space
		glm::mat4 invCam = glm::inverse(camera->projectionMatrix * camera->viewMatrix);
		for (uint32_t j = 0; j < 8; j++) {
			glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[j], 1.0f); //
			frustumCorners[j] = invCorner / invCorner.w;
		}

		for (uint32_t j = 0; j < 4; j++) {
			glm::vec3 dist = frustumCorners[j + 4] - frustumCorners[j];
			frustumCorners[j + 4] = frustumCorners[j] + (dist * splitDist);
			frustumCorners[j] = frustumCorners[j] + (dist * lastSplitDist);
		}

		// Get frustum center
		glm::vec3 frustumCenter = glm::vec3(0.0f);
		for (uint32_t j = 0; j < 8; j++) {
			frustumCenter += frustumCorners[j];
		}
		frustumCenter /= 8.0f;

		float radius = 0.0f;
		for (uint32_t j = 0; j < 8; j++) {
			float distance = glm::length(frustumCorners[j] - frustumCenter);
			radius = glm::max(radius, distance);
		}
		radius = std::ceil(radius * 16.0f) / 16.0f;

		glm::vec3 maxExtents = glm::vec3(radius);
		glm::vec3 minExtents = -maxExtents;

		glm::vec3 lightDir = glm::normalize(-transform.position);
		glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 lightOrthoMatrix = glm::orthoZO(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);

		// Store split distance and matrix in cascade
		cascades[currentFrame][i].splitDepth = (camera->getNearPlane() + splitDist * clipRange) * -1.0f;
		cascades[currentFrame][i].viewProjectionMatrix = lightOrthoMatrix * lightViewMatrix;

		lastSplitDist = shadowCascadeLevels[i];
	}

	UBO ubo{};
	for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
		ubo.cascadeMVPUniform[i] = cascades[currentFrame][i].viewProjectionMatrix;
	}

	memcpy(mappedBuffer[currentFrame], &ubo, sizeof(ubo));
}

void DirectionalLight::genShadowMap(FPSCam* camera, VkDescriptorSetLayout* modelMatrixDescriptorSet, int framesInFlight) {
	width_ = 4096;
	height_ = 4096;
	
	imageFormat_ = findSupportedFormat(pDevHelper_->gpu_);
	cascades.resize(framesInFlight);
	shadowCascadeLevels.resize(SHADOW_MAP_CASCADE_COUNT);
	mappedBuffer.resize(framesInFlight);

	cascadeSplitLambda = 0.91f;

	createFrameBuffer(framesInFlight); // includes createRenderPass. CreateRenderPass includes creating image, image view, and image sampler

	createSMDescriptors(camera, framesInFlight);
}

DirectionalLight::DirectionalLight(glm::vec3 lPos) {
	this->transform.position = lPos;
}

void DirectionalLight::setup(DeviceHelper* devHelper, VkQueue* graphicsQueue, VkCommandPool* cmdPool, float swapChainWidth, float swapChainHeight) {
	this->pDevHelper_ = devHelper;
	this->pGraphicsQueue_ = graphicsQueue;
	this->pCommandPool_ = cmdPool;
	this->swapChainHeight = swapChainHeight;
	this->swapChainWidth = swapChainWidth;
}