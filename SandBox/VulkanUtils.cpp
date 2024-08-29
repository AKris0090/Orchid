#include "VulkanUtils.h"

/// <summary>
/// Pipeline Helper Classes
/// </summary>

static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        std::cout << "failed to open file" << std::endl;
        std::_Xruntime_error("");
    }

    size_t fileSize = file.tellg();
    std::vector<char> buffer((size_t)file.tellg());
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

static VkShaderModule createShaderModule(const VkDevice device, const std::vector<char>& binary) {
    VkShaderModuleCreateInfo shaderModuleCInfo{};
    shaderModuleCInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCInfo.codeSize = binary.size();
    shaderModuleCInfo.pCode = reinterpret_cast<const uint32_t*>(binary.data());

    VkShaderModule shaderMod;
    if (vkCreateShaderModule(device, &shaderModuleCInfo, nullptr, &shaderMod)) {
        std::_Xruntime_error("Failed to create a shader module!");
    }

    return shaderMod;
}

VulkanPipelineBuilder::VulkanShaderModule::VulkanShaderModule(const VkDevice& device, const std::string shaderPath) {
    this->device = device;
    this->module = createShaderModule(device, readFile(shaderPath));
}

void VulkanPipelineBuilder::generateDefaultPipelineData(DeviceHelper* devHelper, const PipelineBuilderInfo& builder) {
    VkPipelineVertexInputStateCreateInfo* pVertexInputState = new VkPipelineVertexInputStateCreateInfo();
    pVertexInputState->sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pVertexInputState->pVertexBindingDescriptions = builder.vertexBindingDescriptions;
    pVertexInputState->vertexBindingDescriptionCount = builder.numVertexBindingDescriptions;
    pVertexInputState->pVertexAttributeDescriptions = builder.vertexAttributeDescriptions;
    pVertexInputState->vertexAttributeDescriptionCount = builder.numVertexAttributeDescriptions;
    info.pVertexInputState = pVertexInputState;

    VkPipelineInputAssemblyStateCreateInfo* pInputAssemblyState = new VkPipelineInputAssemblyStateCreateInfo();
    pInputAssemblyState->sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pInputAssemblyState->topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pInputAssemblyState->primitiveRestartEnable = VK_FALSE;
    info.pInputAssemblyState = pInputAssemblyState;

    info.pTessellationState = nullptr;

    VkPipelineViewportStateCreateInfo* pViewportState = new VkPipelineViewportStateCreateInfo();
    pViewportState->sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pViewportState->viewportCount = 1;
    pViewportState->scissorCount = 1;
    info.pViewportState = pViewportState;

    VkPipelineRasterizationStateCreateInfo* pRasterizationState = new VkPipelineRasterizationStateCreateInfo();
    pRasterizationState->sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pRasterizationState->depthClampEnable = VK_FALSE;
    pRasterizationState->rasterizerDiscardEnable = VK_FALSE;
    pRasterizationState->polygonMode = VK_POLYGON_MODE_FILL;
    pRasterizationState->cullMode = VK_CULL_MODE_BACK_BIT;
    pRasterizationState->frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    pRasterizationState->depthBiasEnable = VK_FALSE;
    pRasterizationState->lineWidth = 1.0f;
    info.pRasterizationState = pRasterizationState;

    VkPipelineMultisampleStateCreateInfo* pMultisampleState = new VkPipelineMultisampleStateCreateInfo();
    pMultisampleState->sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pMultisampleState->rasterizationSamples = devHelper->msaaSamples_;
    info.pMultisampleState = pMultisampleState;

    VkPipelineDepthStencilStateCreateInfo* pDepthStencilState = new VkPipelineDepthStencilStateCreateInfo();
    pDepthStencilState->sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pDepthStencilState->depthTestEnable = VK_TRUE;
    pDepthStencilState->depthWriteEnable = VK_FALSE;
    pDepthStencilState->depthCompareOp = VK_COMPARE_OP_EQUAL;
    pDepthStencilState->depthBoundsTestEnable = VK_FALSE;
    pDepthStencilState->stencilTestEnable = VK_FALSE;
    pDepthStencilState->minDepthBounds = 0.0f;
    pDepthStencilState->maxDepthBounds = 1.0f;
    info.pDepthStencilState = pDepthStencilState;

    info.blendAttachments[0].blendEnable = VK_FALSE;
    info.blendAttachments[0].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    info.blendAttachments[1].blendEnable = VK_FALSE;
    info.blendAttachments[1].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo* pColorBlendState = new VkPipelineColorBlendStateCreateInfo();
    pColorBlendState->sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pColorBlendState->attachmentCount = info.blendAttachments.size();
    pColorBlendState->pAttachments = info.blendAttachments.data();
    info.pColorBlendState = pColorBlendState;

    VkPipelineDynamicStateCreateInfo* pDynamicState = new VkPipelineDynamicStateCreateInfo();
    pDynamicState->sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    pDynamicState->dynamicStateCount = info.dynaStates.size();
    pDynamicState->pDynamicStates = info.dynaStates.data();
    info.pDynamicState = pDynamicState;
}

VulkanPipelineBuilder::VulkanPipelineBuilder(const VkDevice& device, const PipelineBuilderInfo& builder, DeviceHelper* devHelper) {
    VkPipelineLayoutCreateInfo pipeLineLayoutCInfo{};
    pipeLineLayoutCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeLineLayoutCInfo.setLayoutCount = builder.numSets;
    pipeLineLayoutCInfo.pSetLayouts = builder.pDescriptorSetLayouts;
    pipeLineLayoutCInfo.pushConstantRangeCount = builder.numRanges;
    pipeLineLayoutCInfo.pPushConstantRanges = builder.pPushConstantRanges;

    this->device_ = device;

    if (vkCreatePipelineLayout(this->device_, &pipeLineLayoutCInfo, nullptr, &this->layout) != VK_SUCCESS) {
        std::cout << "nah you buggin" << std::endl;
        std::_Xruntime_error("Failed to create pipeline layout!");
    }

    generateDefaultPipelineData(devHelper, builder);
}

void VulkanPipelineBuilder::generate(const PipelineBuilderInfo& builder, const VkRenderPass& renderPass) {
    VkGraphicsPipelineCreateInfo graphicsPipelineCInfo{};

    std::vector<VkPipelineShaderStageCreateInfo> stages(builder.numStages);

    VkPipelineShaderStageCreateInfo vertexStageCInfo{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = builder.pShaderStages[0].module;
    stages[0].pName = "main";

    for (int i = 1; i < builder.numStages; i++) {
        stages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[i].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[i].module = builder.pShaderStages[i].module;
        stages[i].pName = "main";
    }

    graphicsPipelineCInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineCInfo.stageCount = builder.numStages;
    graphicsPipelineCInfo.pStages = stages.data();

    graphicsPipelineCInfo.pVertexInputState = info.pVertexInputState;
    graphicsPipelineCInfo.pInputAssemblyState = info.pInputAssemblyState;
    graphicsPipelineCInfo.pViewportState = info.pViewportState;
    graphicsPipelineCInfo.pRasterizationState = info.pRasterizationState;
    graphicsPipelineCInfo.pMultisampleState = info.pMultisampleState;
    graphicsPipelineCInfo.pDepthStencilState = info.pDepthStencilState;
    graphicsPipelineCInfo.pColorBlendState = info.pColorBlendState;
    graphicsPipelineCInfo.pDynamicState = info.pDynamicState;

    graphicsPipelineCInfo.layout = this->layout;
    graphicsPipelineCInfo.renderPass = renderPass;
    graphicsPipelineCInfo.subpass = 0;
    graphicsPipelineCInfo.basePipelineHandle = VK_NULL_HANDLE;

    VkResult res3 = vkCreateGraphicsPipelines(this->device_, VK_NULL_HANDLE, 1, &graphicsPipelineCInfo, nullptr, &(this->pipeline));
    if (res3 != VK_SUCCESS) {
        std::cout << "failed to create the graphics pipeline" << std::endl;
        std::_Xruntime_error("Failed to create the graphics pipeline!");
    }

    delete(info.pVertexInputState);
    delete(info.pInputAssemblyState);
    delete(info.pTessellationState);
    delete(info.pViewportState);
    delete(info.pRasterizationState);
    delete(info.pMultisampleState);
    delete(info.pDepthStencilState);
    delete(info.pColorBlendState);
    delete(info.pDynamicState);

    for (int i = 0; i < builder.numStages; i++) {
        vkDestroyShaderModule(this->device_, builder.pShaderStages[i].module, nullptr);
    }
}

VulkanDescriptorLayoutBuilder::VulkanDescriptorLayoutBuilder(DeviceHelper* devHelper, std::vector<VulkanDescriptorLayoutBuilder::BindingStruct> bindings) {
    this->device = devHelper->device_;

    std::vector<VkDescriptorSetLayoutBinding> descriptorWrites;
    descriptorWrites.resize(bindings.size());
    for (int i = 0; i < bindings.size(); i++) {
        descriptorWrites[i].binding = i;
        descriptorWrites[i].descriptorType = bindings[i].descriptorType;
        descriptorWrites[i].descriptorCount = 1;
        descriptorWrites[i].stageFlags = bindings[i].stageBits;
        descriptorWrites[i].pImmutableSamplers = nullptr;
    }

    VkDescriptorSetLayoutCreateInfo layoutCInfo{};
    layoutCInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCInfo.bindingCount = descriptorWrites.size();
    layoutCInfo.pBindings = descriptorWrites.data();

    if (vkCreateDescriptorSetLayout(devHelper->device_, &layoutCInfo, nullptr, &layout) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create descriptor set layout!");
    }

}