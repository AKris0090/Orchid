#include "DirectionalLight.h"

#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

std::vector<glm::vec4> DirectionalLight::getFrustrumWorldCoordinates(const glm::mat4& proj, const glm::mat4& view) {
	const auto inv = glm::inverse(proj * view);

	std::vector<glm::vec4> frustumCorners;
	for (unsigned int x = 0; x < 2; ++x)
	{
		for (unsigned int y = 0; y < 2; ++y)
		{
			for (unsigned int z = 0; z < 2; ++z)
			{
				const glm::vec4 pt =
					inv * glm::vec4(
						2.0f * x - 1.0f,
						2.0f * y - 1.0f,
						2.0f * z - 1.0f,
						1.0f);
				frustumCorners.push_back(pt / pt.w);
			}
		}
	}

	return frustumCorners;
}

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

void DirectionalLight::findDepthFormat(VkPhysicalDevice GPU_) {
	imageFormat_ = findSupportedFormat(GPU_, { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkFormat DirectionalLight::findSupportedFormat(VkPhysicalDevice GPU_, const std::vector<VkFormat>& potentialFormats, VkImageTiling tiling, VkFormatFeatureFlags features) {
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
void DirectionalLight::createRenderPass() {
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
void DirectionalLight::createFrameBuffer() {
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
	depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
	depthStencilView.format = imageFormat_;
	depthStencilView.subresourceRange = {};
	depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	depthStencilView.subresourceRange.baseMipLevel = 0;
	depthStencilView.subresourceRange.levelCount = 1;
	depthStencilView.subresourceRange.baseArrayLayer = 0;
	depthStencilView.subresourceRange.layerCount = SHADOW_MAP_CASCADE_COUNT;
	depthStencilView.image = offscreen.image;
	vkCreateImageView(device_, &depthStencilView, nullptr, &sMImageView_);

	createRenderPass();

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
		vkCreateImageView(device_, &viewInfo, nullptr, &cascades[i].imageView);

		VkFramebufferCreateInfo fbufCreateInfo{};
		fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		fbufCreateInfo.renderPass = sMRenderpass_;
		fbufCreateInfo.attachmentCount = 1;
		fbufCreateInfo.pAttachments = &cascades[i].imageView;
		fbufCreateInfo.width = width_;
		fbufCreateInfo.height = height_;
		fbufCreateInfo.layers = 1;
		vkCreateFramebuffer(device_, &fbufCreateInfo, nullptr, &cascades[i].frameBuffer);
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
	vkCreateSampler(device_, &sampler, nullptr, &sMImageSampler_);
}

// CODE PARTIALLY FROM: https://github.com/SaschaWillems/Vulkan/blob/master/examples/shadowmapping/shadowmapping.cpp
void DirectionalLight::createSMDescriptors(glm::mat4 camProj, glm::mat4 camView, float camNear, float camFar, float aspectRatio) {
	VkDeviceSize bufferSize = sizeof(UBO);

	uniformBuffer.resize(1);
	uniformMemory.resize(1);
	mappedBuffer.resize(1);

	pDevHelper_->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffer[0], uniformMemory[0]);
	VkResult res1 = vkMapMemory(pDevHelper_->getDevice(), uniformMemory[0], 0, VK_WHOLE_SIZE, 0, &mappedBuffer[0]);
	
	updateUniBuffers(camProj, camView, camNear, camFar, aspectRatio);

	VkDescriptorSetLayoutBinding UBOLayoutBinding{};
	UBOLayoutBinding.binding = 0;
	UBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	UBOLayoutBinding.descriptorCount = 1;
	UBOLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	UBOLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo layoutCInfo{};
	layoutCInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCInfo.bindingCount = 1;
	layoutCInfo.pBindings = &(UBOLayoutBinding);

	vkCreateDescriptorSetLayout(device_, &layoutCInfo, nullptr, &cascadeSetLayout);

	std::array<VkDescriptorPoolSize, 1> poolSizes{};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = SHADOW_MAP_CASCADE_COUNT;

	VkDescriptorPoolCreateInfo poolCInfo{};
	poolCInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolCInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolCInfo.pPoolSizes = poolSizes.data();
	poolCInfo.maxSets = SHADOW_MAP_CASCADE_COUNT;

	if (vkCreateDescriptorPool(device_, &poolCInfo, nullptr, &sMDescriptorPool_) != VK_SUCCESS) {
		std::_Xruntime_error("Failed to create the descriptor pool!");
	}

	VkDescriptorSetAllocateInfo allocateInfo{};
	allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocateInfo.descriptorPool = sMDescriptorPool_;
	allocateInfo.descriptorSetCount = 1;
	allocateInfo.pSetLayouts = &cascadeSetLayout;

	for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
		VkResult res2 = vkAllocateDescriptorSets(device_, &allocateInfo, &cascades[i].descriptorSet);

		VkDescriptorBufferInfo descriptorBufferInfo{};
		descriptorBufferInfo.buffer = uniformBuffer[0];
		descriptorBufferInfo.offset = 0;
		descriptorBufferInfo.range = sizeof(UBO);

		VkWriteDescriptorSet bufferWriteSet{};
		bufferWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		bufferWriteSet.dstSet = cascades[i].descriptorSet;
		bufferWriteSet.dstBinding = 0;
		bufferWriteSet.dstArrayElement = 0;
		bufferWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		bufferWriteSet.descriptorCount = 1;
		bufferWriteSet.pBufferInfo = &descriptorBufferInfo;

		std::array<VkWriteDescriptorSet, 1> descriptors = { bufferWriteSet };

		vkUpdateDescriptorSets(device_, 1, descriptors.data(), 0, NULL);
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

void DirectionalLight::createPipeline() {
    VkPushConstantRange pcRange{};
    pcRange.offset = 0;
    pcRange.size = sizeof(GLTFObj::cascadeMVP);
    pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPipelineLayoutCreateInfo pipeLineLayoutCInfo{};
    pipeLineLayoutCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeLineLayoutCInfo.setLayoutCount = 1;
	pipeLineLayoutCInfo.pSetLayouts = &cascadeSetLayout;
	pipeLineLayoutCInfo.pushConstantRangeCount = 1;
	pipeLineLayoutCInfo.pPushConstantRanges = &pcRange;

    if (vkCreatePipelineLayout(device_, &pipeLineLayoutCInfo, nullptr, &(sMPipelineLayout_)) != VK_SUCCESS) {
        std::cout << "nah you buggin" << std::endl;
        std::_Xruntime_error("Failed to create brdfLUT pipeline layout!");
    }

    std::vector<char> sMVertShader = readFile("C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/spv/shadowMap.spv");
	std::vector<char> sMFragShader = readFile("C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/spv/shadowMapFrag.spv");

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
	rasterizerCInfo.depthClampEnable = VK_TRUE;
	rasterizerCInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizerCInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizerCInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
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

void DirectionalLight::createAnimatedPipeline(VkDescriptorSetLayout animatedDescLayout) {
	VkPushConstantRange pcRange{};
	pcRange.offset = 0;
	pcRange.size = sizeof(GLTFObj::cascadeMVP);
	pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	VkDescriptorSetLayout descSetLayouts[] = { animatedDescLayout, cascadeSetLayout };

	VkPipelineLayoutCreateInfo pipeLineLayoutCInfo{};
	pipeLineLayoutCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeLineLayoutCInfo.setLayoutCount = 2;
	pipeLineLayoutCInfo.pSetLayouts = descSetLayouts;
	pipeLineLayoutCInfo.pushConstantRangeCount = 1;
	pipeLineLayoutCInfo.pPushConstantRanges = &pcRange;

	if (vkCreatePipelineLayout(device_, &pipeLineLayoutCInfo, nullptr, &(animatedSmPipelineLayout)) != VK_SUCCESS) {
		std::cout << "nah you buggin" << std::endl;
		std::_Xruntime_error("Failed to create brdfLUT pipeline layout!");
	}

	std::vector<char> sMVertShader = readFile("C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/spv/animShadowMapVert.spv");
	std::vector<char> sMFragShader = readFile("C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/spv/shadowMapFrag.spv");

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

	/*	VkPipelineRasterizationStateCreateInfo rasterizerCInfo{};
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
	depthStencilCInfo.maxDepthBounds = 1.0f;*/

VkPipelineRasterizationStateCreateInfo rasterizerCInfo{};
	rasterizerCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerCInfo.pNext = nullptr;
	rasterizerCInfo.flags = 0;
	rasterizerCInfo.depthClampEnable = VK_FALSE;
	rasterizerCInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizerCInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizerCInfo.cullMode = VK_CULL_MODE_FRONT_BIT;
	rasterizerCInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizerCInfo.depthBiasEnable = VK_TRUE;
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
	depthStencilCInfo.minDepthBounds = 1.0f;
	depthStencilCInfo.maxDepthBounds = 0.0f;

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
	auto attributeDescriptions = MeshHelper::Vertex::getAnimatedPositionDescription();

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

	shadowMapPipelineCInfo.layout = animatedSmPipelineLayout;

	shadowMapPipelineCInfo.renderPass = sMRenderpass_;
	shadowMapPipelineCInfo.subpass = 0;

	shadowMapPipelineCInfo.basePipelineHandle = VK_NULL_HANDLE;

	vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &shadowMapPipelineCInfo, nullptr, &animatedSMPipeline);

	//std::cout << "pipeline created" << std::endl;
}


void DirectionalLight::endCommandBuffer(VkDevice device_, VkCommandBuffer cmdBuff, VkQueue* pGraphicsQueue_, VkCommandPool* pCommandPool_) {
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
DirectionalLight::PostRenderPacket DirectionalLight::render(VkCommandBuffer cmdBuf, uint32_t cascadeIndex) {
    VkClearValue clearValues[1];
    clearValues[0].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = sMRenderpass_;
    renderPassBeginInfo.renderArea.extent.width = width_;
    renderPassBeginInfo.renderArea.extent.height = height_;
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = clearValues;
	renderPassBeginInfo.framebuffer = cascades[cascadeIndex].frameBuffer;

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

	vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_GRAPHICS, sMPipeline_);

	return { renderPassBeginInfo, sMPipeline_, sMPipelineLayout_, cmdBuf };
}


std::vector<glm::vec4> getFrustumCornersWorldSpace(const glm::mat4& projview)
{
	const auto inv = glm::inverse(projview);

	std::vector<glm::vec4> frustumCorners;
	for (unsigned int x = 0; x < 2; ++x)
	{
		for (unsigned int y = 0; y < 2; ++y)
		{
			for (unsigned int z = 0; z < 2; ++z)
			{
				const glm::vec4 pt = inv * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
				frustumCorners.push_back(pt / pt.w);
			}
		}
	}

	return frustumCorners;
}

std::vector<glm::vec4> getFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view)
{
	return getFrustumCornersWorldSpace(proj * view);
}

glm::mat4 DirectionalLight::getLightSpaceMatrix(float nearPlane, float farPlane, glm::mat4 camView, float aspectRatio) {
	const auto proj = glm::perspective(75.0f,aspectRatio, nearPlane,farPlane);
	const auto corners = getFrustumCornersWorldSpace(proj, camView);

	glm::vec3 center = glm::vec3(0, 0, 0);
	for (const auto& v : corners)
	{
		center += glm::vec3(v);
	}
	center /= corners.size();

	const auto lightView = glm::lookAt(center + glm::vec3(*lightPos), center, glm::vec3(0.0f, 1.0f, 0.0f));

	float minX = std::numeric_limits<float>::max();
	float maxX = std::numeric_limits<float>::lowest();
	float minY = std::numeric_limits<float>::max();
	float maxY = std::numeric_limits<float>::lowest();
	float minZ = std::numeric_limits<float>::max();
	float maxZ = std::numeric_limits<float>::lowest();
	for (const auto& v : corners)
	{
		const auto trf = lightView * v;
		minX = std::min(minX, trf.x);
		maxX = std::max(maxX, trf.x);
		minY = std::min(minY, trf.y);
		maxY = std::max(maxY, trf.y);
		minZ = std::min(minZ, trf.z);
		maxZ = std::max(maxZ, trf.z);
	}

	// Tune this parameter according to the scene
	constexpr float zMult = 10.0f;
	if (minZ < 0)
	{
		minZ *= zMult;
	}
	else
	{
		minZ /= zMult;
	}
	if (maxZ < 0)
	{
		maxZ /= zMult;
	}
	else
	{
		maxZ *= zMult;
	}

	glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
	lightProjection[1][1] *= -1;
	return lightProjection * lightView;
}

void DirectionalLight::updateUniBuffers(glm::mat4 cameraProj, glm::mat4 camView, float cameraNearPlane, float cameraFarPlane, float aspectRatio) {
	//getLightSpaceMatrices(cameraProj, camView);
	//for (int i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
	//	uniShadow.cascadeMVP[i] = cascades[i].viewProjectionMatrix;
	//}

	float clipRange = cameraFarPlane - cameraNearPlane;

	float minZ = cameraNearPlane;
	float maxZ = cameraNearPlane + clipRange;

	float range = maxZ - minZ;
	float ratio = maxZ / minZ;

	std::vector<float> shadowCascadeLevels{};
	shadowCascadeLevels.resize(4);

	for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
		float p = (i + 1) / static_cast<float>(SHADOW_MAP_CASCADE_COUNT);
		float log = minZ * std::pow(ratio, p);
		float uniform = minZ + range * p;
		float d = cascadeSplitLambda * (log - uniform) + uniform;
		shadowCascadeLevels[i] = (d - cameraNearPlane) / clipRange;
	}

	//std::vector<glm::mat4> ret;
	//for (size_t i = 0; i < shadowCascadeLevels.size(); i++)
	//{
	//	if (i == 0)
	//	{
	//		ret.push_back(getLightSpaceMatrix(cameraNearPlane, shadowCascadeLevels[i], camView, aspectRatio));
	//	}
	//	else if (i < shadowCascadeLevels.size() - 1)
	//	{
	//		ret.push_back(getLightSpaceMatrix(shadowCascadeLevels[i], shadowCascadeLevels[i + 1], camView, aspectRatio));
	//	}
	//	else
	//	{
	//		ret.push_back(getLightSpaceMatrix(shadowCascadeLevels[i], cameraFarPlane, camView, aspectRatio));
	//	}
	//}
	// 
	// Calculate orthographic projection matrix for each cascade
	float lastSplitDist = 0.0;
	for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
		float splitDist = shadowCascadeLevels[i];

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
		glm::mat4 invCam = glm::inverse(cameraProj * camView);
		for (uint32_t j = 0; j < 8; j++) {
			glm::vec4 invCorner = invCam * glm::vec4(frustumCorners[j], 1.0f);
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

		glm::vec3 lightDir = normalize(-(*lightPos));
		glm::mat4 lightViewMatrix = glm::lookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 lightOrthoMatrix = glm::orthoZO(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);

		// Store split distance and matrix in cascade
		cascades[i].splitDepth = (cameraNearPlane + splitDist * clipRange) * -1.0f;
		lightOrthoMatrix[1][1] *= -1.0f;
		cascades[i].viewProjectionMatrix = lightOrthoMatrix * lightViewMatrix;

		lastSplitDist = shadowCascadeLevels[i];
	}

	UBO ubo;
	for (uint32_t i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
		ubo.cascadeMVP[i] = cascades[i].viewProjectionMatrix;
	}

	memcpy(mappedBuffer[0], &ubo, sizeof(ubo));
}

void DirectionalLight::genShadowMap(glm::mat4 camProj, glm::mat4 camView, float camNear, float camFar, float aspectRatio) {
	width_ = 4096;
	height_ = 4096;
	zNear = 1.f;
	zFar = 100.0f;
	imageFormat_ = VK_FORMAT_D16_UNORM;
	cascadeSplitLambda = 0.95f;

	createFrameBuffer(); // includes createRenderPass. CreateRenderPass includes creating image, image view, and image sampler

	createSMDescriptors(camProj, camView, camNear, camFar, aspectRatio);

	createPipeline();
}

DirectionalLight::DirectionalLight(DeviceHelper* devHelper, VkQueue* graphicsQueue, VkCommandPool* cmdPool, glm::vec4* lPos, std::vector<GLTFObj*> pModels_, uint32_t numModels_, float swapChainWidth, float swapChainHeight) {
	this->pDevHelper_ = devHelper;
	this->device_ = devHelper->getDevice();
	this->pGraphicsQueue_ = graphicsQueue;
	this->pCommandPool_ = cmdPool;
	this->lightPos = lPos;
	this->numModels_ = numModels_;
	this->pModels_ = pModels_;
	this->imageFormat_ = VK_FORMAT_D16_UNORM;
	this->swapChainHeight = swapChainHeight;
	this->swapChainWidth = swapChainWidth;
}