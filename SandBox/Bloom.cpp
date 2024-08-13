#include "Bloom.h"

void BloomHelper::createImageViews() {
	imageViewMipChain.resize(BLOOM_LEVELS);
	imageViewMipChain[0] = *emissionImageView_;
	for (int mip = 1; mip < BLOOM_LEVELS; mip++) {
		VkImageViewCreateInfo imageViewCInfo{};
		imageViewCInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCInfo.image = *emissionImage;
		imageViewCInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCInfo.format = emissionImageFormat;
		imageViewCInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCInfo.subresourceRange.baseMipLevel = mip;
		imageViewCInfo.subresourceRange.levelCount = 1;
		imageViewCInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCInfo.subresourceRange.layerCount = 1;

		VkResult res = vkCreateImageView(pDevHelper_->device_, &imageViewCInfo, nullptr, &imageViewMipChain[mip]);
		if (res != VK_SUCCESS) {
			std::cout << "could not create bloom image view" << std::endl;
			std::_Xruntime_error("could not create bloom image view");
		}
	}
}

void BloomHelper::createAttachments() {
	int mip = 1;

	VkFormat format = emissionImageFormat;
	VkExtent2D extent = emissionImageExtent;

	for (int i = 0; i < NUMBER_OF_DOWNSAMPLED_IMAGES; i++) {
		VkExtent2D currentMipExtent{ extent.width >> mip, extent.height >> mip };

		Attachment attachment{
			imageViewMipChain[mip],
			format,
			currentMipExtent,
			VK_ATTACHMENT_LOAD_OP_CLEAR,
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		};
		mipChainAttachmentsDown.push_back(attachment);
		++mip;
	}

	mip = NUMBER_OF_DOWNSAMPLED_IMAGES - 1;
	for (int i = 0; i < NUMBER_OF_DOWNSAMPLED_IMAGES; i++) {
		VkExtent2D currentMipExtent{ extent.width >> mip, extent.height >> mip };

		Attachment attachment{
			imageViewMipChain[mip],
			format,
			currentMipExtent,
			VK_ATTACHMENT_LOAD_OP_LOAD,
			VK_ATTACHMENT_STORE_OP_STORE,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
		};
		mipChainAttachmentsUp.push_back(attachment);
		--mip;
	}
}

void BloomHelper::createRenderPasses() {
	{ // up renderpass
		VkAttachmentDescription aDescription{};
		aDescription.format = mipChainAttachmentsUp[0].format;
		aDescription.samples = VK_SAMPLE_COUNT_1_BIT;
		aDescription.loadOp = mipChainAttachmentsUp[0].loadOp;
		aDescription.storeOp = mipChainAttachmentsUp[0].storeOp;
		aDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		aDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		aDescription.initialLayout = mipChainAttachmentsUp[0].initialImageLayout;
		aDescription.finalLayout = mipChainAttachmentsUp[0].finalImageLayout;

		VkAttachmentReference attachmentRef{};
		attachmentRef.attachment = 0;
		attachmentRef.layout = mipChainAttachmentsUp[0].subImageLayout;

		VkSubpassDescription subpass = {};
		subpass.flags = 0;
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.inputAttachmentCount = 0;
		subpass.pInputAttachments = nullptr;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &attachmentRef;
		subpass.pResolveAttachments = nullptr;
		subpass.pDepthStencilAttachment = nullptr;
		subpass.preserveAttachmentCount = 0;
		subpass.pPreserveAttachments = nullptr;

		std::array<VkSubpassDependency, 2> dependencies{};
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = 0;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &aDescription;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 2;
		renderPassInfo.pDependencies = dependencies.data();

		VkResult res = vkCreateRenderPass(pDevHelper_->device_, &renderPassInfo, nullptr, &bloomRenderPassUp);
		if (res != VK_SUCCESS) {
			std::cout << "could not create up render pass" << std::endl;
			std::_Xruntime_error("could not create up render pass");
		}
	}

	{ // down renderpass
		VkAttachmentDescription aDescription{};
		aDescription.format = mipChainAttachmentsDown[0].format;
		aDescription.samples = VK_SAMPLE_COUNT_1_BIT;
		aDescription.loadOp = mipChainAttachmentsDown[0].loadOp;
		aDescription.storeOp = mipChainAttachmentsDown[0].storeOp;
		aDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		aDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		aDescription.initialLayout = mipChainAttachmentsDown[0].initialImageLayout;
		aDescription.finalLayout = mipChainAttachmentsDown[0].finalImageLayout;

		VkAttachmentReference attachmentRef{};
		attachmentRef.attachment = 0;
		attachmentRef.layout = mipChainAttachmentsDown[0].subImageLayout;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &attachmentRef;

		std::array<VkSubpassDependency, 2> dependencies{};
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[0].srcAccessMask = 0;
		dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		VkRenderPassCreateInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &aDescription;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 2;
		renderPassInfo.pDependencies = dependencies.data();

		VkResult res = vkCreateRenderPass(pDevHelper_->device_, &renderPassInfo, nullptr, &bloomRenderPassDown);
		if (res != VK_SUCCESS) {
			std::cout << "could not create down render pass" << std::endl;
			std::_Xruntime_error("could not create down render pass");
		}
	}
}

void BloomHelper::createFramebuffersDown() {
	frameBuffersDown.resize(NUMBER_OF_DOWNSAMPLED_IMAGES);
	for (int i = 0; i < NUMBER_OF_DOWNSAMPLED_IMAGES; i++)
	{
		auto& attachment = mipChainAttachmentsDown[i]; // m_attachmentsDown[0] -> mip level 1
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = bloomRenderPassDown;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = &(attachment.imageView);
		framebufferInfo.width = attachment.imageExtent.width;
		framebufferInfo.height = attachment.imageExtent.height;
		frameBuffersDown[i].extent.width = attachment.imageExtent.width;
		frameBuffersDown[i].extent.height = attachment.imageExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(pDevHelper_->device_, &framebufferInfo, nullptr, &(frameBuffersDown[i].frameBuffer)) != VK_SUCCESS)
		{
			std::cout << "could not make frameuffer" << std::endl;
			std::_Xruntime_error("could not make framebuffer");
		}
	}
}

void BloomHelper::createFramebuffersUp() {
	frameBuffersUp.resize(NUMBER_OF_DOWNSAMPLED_IMAGES);
	for (int i = 0; i < NUMBER_OF_DOWNSAMPLED_IMAGES; i++)
	{
		auto& attachment = mipChainAttachmentsUp[i]; // m_attachmentsDown[0] -> mip level 1
		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = bloomRenderPassUp;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = &(attachment.imageView);
		framebufferInfo.width = attachment.imageExtent.width;
		framebufferInfo.height = attachment.imageExtent.height;
		frameBuffersUp[i].extent.width = attachment.imageExtent.width;
		frameBuffersUp[i].extent.height = attachment.imageExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(pDevHelper_->device_, &framebufferInfo, nullptr, &(frameBuffersUp[i].frameBuffer)) != VK_SUCCESS)
		{
			std::cout << "could not make frameuffer" << std::endl;
			std::_Xruntime_error("could not make framebuffer");
		}
	}
}

void BloomHelper::createDescriptors() {
	VkDescriptorSetLayoutBinding imageLayoutBinding{};
	imageLayoutBinding.binding = 0;
	imageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	imageLayoutBinding.descriptorCount = 1;
	imageLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	imageLayoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo layoutCInfo{};
	layoutCInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutCInfo.bindingCount = 1;
	layoutCInfo.pBindings = &imageLayoutBinding;

	if (vkCreateDescriptorSetLayout(pDevHelper_->device_, &layoutCInfo, nullptr, &bloomSetLayout) != VK_SUCCESS) {
		std::_Xruntime_error("Failed to create the uniform descriptor set layout!");
	}

	VkSamplerCreateInfo samplerCreateInfo{};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.pNext = nullptr;
	samplerCreateInfo.flags = 0;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	samplerCreateInfo.mipLodBias = 0.0f;
	samplerCreateInfo.anisotropyEnable = VK_TRUE;
	samplerCreateInfo.maxAnisotropy = 16.0;
	samplerCreateInfo.compareEnable = VK_FALSE;
	samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerCreateInfo.minLod = 0.0f;
	samplerCreateInfo.maxLod = 1.0f;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;

	if (vkCreateSampler(pDevHelper_->device_, &samplerCreateInfo, nullptr, &bloomSampler) != VK_SUCCESS) {
		std::_Xruntime_error("Failed to create the bloom image sampler!");
	}

	bloomSets.resize(BLOOM_LEVELS);

	std::vector<VkDescriptorSetLayout> setLayouts;
	for (int i = 0; i < BLOOM_LEVELS; i++) {
		setLayouts.push_back(bloomSetLayout);
	}

	VkDescriptorSetAllocateInfo bloomAllocateInfo{};
	bloomAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	bloomAllocateInfo.descriptorPool = pDevHelper_->descPool_;
	bloomAllocateInfo.descriptorSetCount = BLOOM_LEVELS;
	bloomAllocateInfo.pSetLayouts = setLayouts.data();

	VkResult res2 = vkAllocateDescriptorSets(pDevHelper_->device_, &bloomAllocateInfo, bloomSets.data());
	if (res2 != VK_SUCCESS) {
		std::cout << res2 << std::endl;
		std::_Xruntime_error("Failed to allocate descriptor sets!");
	}

	for (int i = 0; i < BLOOM_LEVELS; i++) {
		VkDescriptorImageInfo descriptorImageInfo{};
		descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		descriptorImageInfo.imageView = imageViewMipChain[i];
		descriptorImageInfo.sampler = bloomSampler;

		VkWriteDescriptorSet descriptorWriteSet{};
		descriptorWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWriteSet.dstSet = bloomSets[i];
		descriptorWriteSet.dstBinding = 0;
		descriptorWriteSet.dstArrayElement = 0;
		descriptorWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWriteSet.descriptorCount = 1;
		descriptorWriteSet.pImageInfo = &descriptorImageInfo;

		vkUpdateDescriptorSets(pDevHelper_->device_, 1, &descriptorWriteSet, 0, nullptr);
	}
}

std::vector<char> BloomHelper::readFile(const std::string& filename) {
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
VkShaderModule BloomHelper::createShaderModule(const std::vector<char>& binary) {
	// We need to specify a pointer to the buffer with the bytecode and the length of the bytecode. Bytecode pointer is a uint32_t pointer
	VkShaderModuleCreateInfo shaderModuleCInfo{};
	shaderModuleCInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCInfo.codeSize = binary.size();
	shaderModuleCInfo.pCode = reinterpret_cast<const uint32_t*>(binary.data());

	VkShaderModule shaderMod;
	if (vkCreateShaderModule(pDevHelper_->device_, &shaderModuleCInfo, nullptr, &shaderMod)) {
		std::_Xruntime_error("Failed to create a shader module!");
	}

	return shaderMod;
}

void BloomHelper::createBloomPipelines() {
	VkPushConstantRange pushConstantRange{};
	pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	pushConstantRange.offset = 0;
	pushConstantRange.size = sizeof(pushConstantBloom);

	// We can use uniform values to make changes to the shaders without having to create them again, similar to global variables
	// Initialize the pipeline layout with another create info struct
	VkPipelineLayoutCreateInfo pipeLineLayoutCInfo{};
	pipeLineLayoutCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeLineLayoutCInfo.setLayoutCount = 1;
	pipeLineLayoutCInfo.pSetLayouts = &bloomSetLayout;
	pipeLineLayoutCInfo.pushConstantRangeCount = 1;
	pipeLineLayoutCInfo.pPushConstantRanges = &pushConstantRange;

	if (vkCreatePipelineLayout(pDevHelper_->device_, &pipeLineLayoutCInfo, nullptr, &bloomPipelineLayout) != VK_SUCCESS) {
		std::cout << "nah you buggin" << std::endl;
		std::_Xruntime_error("Failed to create pipeline layout!");
	}

	{// down pipeline
		// Read the file for the bytecodfe of the shaders
		std::vector<char> vertexShader = readFile("C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/spv/screenQuadVert.spv");
		std::vector<char> fragmentShader = readFile("C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/spv/downSample.spv");

		std::cout << "read files" << std::endl;

		// Wrap the bytecode with VkShaderModule objects
		VkShaderModule vertexShaderModule = createShaderModule(vertexShader);
		VkShaderModule fragmentShaderModule = createShaderModule(fragmentShader);

		//Create the shader information struct to begin actuall using the shader
		VkPipelineShaderStageCreateInfo vertexStageCInfo{};
		vertexStageCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertexStageCInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexStageCInfo.module = vertexShaderModule;
		vertexStageCInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragmentStageCInfo{};
		fragmentStageCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragmentStageCInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentStageCInfo.module = fragmentShaderModule;
		fragmentStageCInfo.pName = "main";

		// Define array to contain the shader create information structs
		VkPipelineShaderStageCreateInfo stages[] = { vertexStageCInfo, fragmentStageCInfo };

		// Describing the format of the vertex data to be passed to the vertex shader
		VkPipelineVertexInputStateCreateInfo vertexInputCInfo{};
		vertexInputCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getPositionAttributeDescription();

		vertexInputCInfo.vertexBindingDescriptionCount = 1;
		vertexInputCInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputCInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());;
		vertexInputCInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

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
		rasterizerCInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizerCInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		// Alter depth values by adding constant or biasing them based on a fragment's slope
		rasterizerCInfo.depthBiasEnable = VK_FALSE;

		// Multisampling information struct
		VkPipelineMultisampleStateCreateInfo multiSamplingCInfo{};
		multiSamplingCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multiSamplingCInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		// Depth and stencil testing would go here, but not doing this for the triangle
		VkPipelineDepthStencilStateCreateInfo depthStencilCInfo{};
		depthStencilCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilCInfo.depthTestEnable = VK_FALSE;
		depthStencilCInfo.depthWriteEnable = VK_FALSE;
		depthStencilCInfo.depthBoundsTestEnable = VK_FALSE;
		depthStencilCInfo.stencilTestEnable = VK_FALSE;
		depthStencilCInfo.minDepthBounds = 0.0f;
		depthStencilCInfo.maxDepthBounds = 1.0f;

		// Color blending - color from fragment shader needs to be combined with color already in the framebuffer
		// If <blendEnable> is set to false, then the color from the fragment shader is passed through to the framebuffer
		// Otherwise, combine with a colorWriteMask to determine the channels that are passed through
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;

		// Array of structures for all of the framebuffers to set blend constants as blend factors
		VkPipelineColorBlendStateCreateInfo colorBlendingCInfo{};
		colorBlendingCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendingCInfo.attachmentCount = 1;
		colorBlendingCInfo.pAttachments = &colorBlendAttachment;

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

		// Combine the shader stages, fixed-function state, pipeline layout, and render pass to create the graphics pipeline
		// First - populate struct with the information
		VkGraphicsPipelineCreateInfo graphicsPipelineCInfo{};
		graphicsPipelineCInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		graphicsPipelineCInfo.stageCount = 2;
		graphicsPipelineCInfo.pStages = stages;

		graphicsPipelineCInfo.pVertexInputState = &vertexInputCInfo;
		graphicsPipelineCInfo.pInputAssemblyState = &inputAssemblyCInfo;
		graphicsPipelineCInfo.pViewportState = &viewportStateCInfo;
		graphicsPipelineCInfo.pRasterizationState = &rasterizerCInfo;
		graphicsPipelineCInfo.pMultisampleState = &multiSamplingCInfo;
		graphicsPipelineCInfo.pDepthStencilState = &depthStencilCInfo;
		graphicsPipelineCInfo.pColorBlendState = &colorBlendingCInfo;
		graphicsPipelineCInfo.pDynamicState = &dynamicStateCInfo;

		graphicsPipelineCInfo.layout = bloomPipelineLayout;

		graphicsPipelineCInfo.renderPass = bloomRenderPassDown;
		graphicsPipelineCInfo.subpass = 0;

		graphicsPipelineCInfo.basePipelineHandle = VK_NULL_HANDLE;

		// Create the object
		VkResult res3 = vkCreateGraphicsPipelines(pDevHelper_->device_, VK_NULL_HANDLE, 1, &graphicsPipelineCInfo, nullptr, &(bloomPipelineDown));
		if (res3 != VK_SUCCESS) {
			std::cout << "failed to create toneMapping graphics pipeline" << std::endl;
			std::_Xruntime_error("Failed to create the graphics pipeline!");
		}
	}

	{ // up pipeline
		// Read the file for the bytecodfe of the shaders
		std::vector<char> vertexShader = readFile("C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/spv/screenQuadVert.spv");
		std::vector<char> fragmentShader = readFile("C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/spv/upSample.spv");

		std::cout << "read files" << std::endl;

		// Wrap the bytecode with VkShaderModule objects
		VkShaderModule vertexShaderModule = createShaderModule(vertexShader);
		VkShaderModule fragmentShaderModule = createShaderModule(fragmentShader);

		//Create the shader information struct to begin actuall using the shader
		VkPipelineShaderStageCreateInfo vertexStageCInfo{};
		vertexStageCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertexStageCInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexStageCInfo.module = vertexShaderModule;
		vertexStageCInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragmentStageCInfo{};
		fragmentStageCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragmentStageCInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentStageCInfo.module = fragmentShaderModule;
		fragmentStageCInfo.pName = "main";

		// Define array to contain the shader create information structs
		VkPipelineShaderStageCreateInfo stages[] = { vertexStageCInfo, fragmentStageCInfo };

		// Describing the format of the vertex data to be passed to the vertex shader
		VkPipelineVertexInputStateCreateInfo vertexInputCInfo{};
		vertexInputCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getPositionAttributeDescription();

		vertexInputCInfo.vertexBindingDescriptionCount = 1;
		vertexInputCInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputCInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());;
		vertexInputCInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

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
		rasterizerCInfo.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizerCInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		// Alter depth values by adding constant or biasing them based on a fragment's slope
		rasterizerCInfo.depthBiasEnable = VK_FALSE;

		// Multisampling information struct
		VkPipelineMultisampleStateCreateInfo multiSamplingCInfo{};
		multiSamplingCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multiSamplingCInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		// Depth and stencil testing would go here, but not doing this for the triangle
		VkPipelineDepthStencilStateCreateInfo depthStencilCInfo{};
		depthStencilCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilCInfo.depthTestEnable = VK_FALSE;
		depthStencilCInfo.depthWriteEnable = VK_FALSE;
		depthStencilCInfo.depthBoundsTestEnable = VK_FALSE;
		depthStencilCInfo.stencilTestEnable = VK_FALSE;
		depthStencilCInfo.minDepthBounds = 0.0f;
		depthStencilCInfo.maxDepthBounds = 1.0f;

		// Color blending - color from fragment shader needs to be combined with color already in the framebuffer
		// If <blendEnable> is set to false, then the color from the fragment shader is passed through to the framebuffer
		// Otherwise, combine with a colorWriteMask to determine the channels that are passed through
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_TRUE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;

		// Array of structures for all of the framebuffers to set blend constants as blend factors
		VkPipelineColorBlendStateCreateInfo colorBlendingCInfo{};
		colorBlendingCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendingCInfo.attachmentCount = 1;
		colorBlendingCInfo.pAttachments = &colorBlendAttachment;

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

		// Combine the shader stages, fixed-function state, pipeline layout, and render pass to create the graphics pipeline
		// First - populate struct with the information
		VkGraphicsPipelineCreateInfo graphicsPipelineCInfo{};
		graphicsPipelineCInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		graphicsPipelineCInfo.stageCount = 2;
		graphicsPipelineCInfo.pStages = stages;

		graphicsPipelineCInfo.pVertexInputState = &vertexInputCInfo;
		graphicsPipelineCInfo.pInputAssemblyState = &inputAssemblyCInfo;
		graphicsPipelineCInfo.pViewportState = &viewportStateCInfo;
		graphicsPipelineCInfo.pRasterizationState = &rasterizerCInfo;
		graphicsPipelineCInfo.pMultisampleState = &multiSamplingCInfo;
		graphicsPipelineCInfo.pDepthStencilState = &depthStencilCInfo;
		graphicsPipelineCInfo.pColorBlendState = &colorBlendingCInfo;
		graphicsPipelineCInfo.pDynamicState = &dynamicStateCInfo;

		graphicsPipelineCInfo.layout = bloomPipelineLayout;

		graphicsPipelineCInfo.renderPass = bloomRenderPassUp;
		graphicsPipelineCInfo.subpass = 0;

		graphicsPipelineCInfo.basePipelineHandle = VK_NULL_HANDLE;

		// Create the object
		VkResult res3 = vkCreateGraphicsPipelines(pDevHelper_->device_, VK_NULL_HANDLE, 1, &graphicsPipelineCInfo, nullptr, &(bloomPipelineUp));
		if (res3 != VK_SUCCESS) {
			std::cout << "failed to create toneMapping graphics pipeline" << std::endl;
			std::_Xruntime_error("Failed to create the graphics pipeline!");
		}
	}
}

void BloomHelper::setViewport(VkCommandBuffer& commandBuffer, VkExtent2D& extent)
{
	VkViewport viewport{};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(extent.width);
	viewport.height = static_cast<float>(extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	VkRect2D scissor{ {0, 0}, extent };
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void BloomHelper::startBloomRenderPass(VkCommandBuffer& commandBuffer, VkRenderPass* renderPass, FrameBufferHolder* framebuffer) {
	VkClearValue clearValues[1];
	clearValues[0].color = { 1.0f, 1.0f, 1.0f, 1.0f };

	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = *renderPass;
	renderPassInfo.framebuffer = framebuffer->frameBuffer;

	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = framebuffer->extent;

	renderPassInfo.clearValueCount = 1;
	renderPassInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void BloomHelper::setupBloom(VkImage* emissionImage, VkImageView* imgView, VkFormat emissionImageFormat, VkExtent2D emissionImageExtent) {
	this->emissionImage = emissionImage;
	this->emissionImageFormat = emissionImageFormat;
	this->emissionImageExtent = emissionImageExtent;
	this->emissionImageView_ = imgView;
	createImageViews();
	createAttachments();
	createRenderPasses();
	createFramebuffersDown();
	createFramebuffersUp();

	createDescriptors();
	createBloomPipelines();
}

BloomHelper::BloomHelper(DeviceHelper* devH) {
	this->pDevHelper_ = devH;
}