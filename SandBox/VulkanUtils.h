#pragma once

#include "MeshHelper.h"

/// <summary>
/// Pipeline Helper Classes
/// </summary>

class VulkanPipelineBuilder {
public:
	class VulkanShaderModule {
	public:
		VkShaderModule module;
		VkDevice device;

		VulkanShaderModule(const VkDevice& device_, const std::string shaderPath);
		~VulkanShaderModule() {
			//vkDestroyShaderModule(this->device, this->module, nullptr);
		};
	};

	struct PipelineBuilderInfo {
		VkDescriptorSetLayout* pDescriptorSetLayouts;
		int numSets;

		VkPushConstantRange* pPushConstantRanges;
		int numRanges;

		VulkanShaderModule* pShaderStages;
		int numStages;

		VkVertexInputAttributeDescription* vertexAttributeDescriptions;
		int numVertexAttributeDescriptions;
		VkVertexInputBindingDescription* vertexBindingDescriptions;
		int numVertexBindingDescriptions;
	};

	struct PipelineInfo {
		std::vector<VkPipelineColorBlendAttachmentState> blendAttachments = std::vector<VkPipelineColorBlendAttachmentState>(2);

		std::vector<VkDynamicState> dynaStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineVertexInputStateCreateInfo* pVertexInputState;
		VkPipelineInputAssemblyStateCreateInfo* pInputAssemblyState;
		VkPipelineTessellationStateCreateInfo* pTessellationState;
		VkPipelineViewportStateCreateInfo* pViewportState;
		VkPipelineRasterizationStateCreateInfo* pRasterizationState;
		VkPipelineMultisampleStateCreateInfo* pMultisampleState;
		VkPipelineDepthStencilStateCreateInfo* pDepthStencilState;
		VkPipelineColorBlendStateCreateInfo* pColorBlendState;
		VkPipelineDynamicStateCreateInfo* pDynamicState;

		PipelineInfo() {};
	};

	PipelineInfo info;
	VkDevice device_;

	VkPipelineLayout layout;
	VkPipeline pipeline;

	VulkanPipelineBuilder() {};
	~VulkanPipelineBuilder() {
		vkDestroyPipeline(this->device_, this->pipeline, nullptr);
		vkDestroyPipelineLayout(this->device_, this->layout, nullptr);
	};

	VulkanPipelineBuilder(const VkDevice& device, const PipelineBuilderInfo& builder, DeviceHelper* devHelper);
	void generate(const PipelineBuilderInfo& builder, const VkRenderPass& renderPass);
private:
	void generateDefaultPipelineData(DeviceHelper* devHelper, const PipelineBuilderInfo& builder);
};