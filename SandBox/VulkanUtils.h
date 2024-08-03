#pragma once

#include "vulkan/vulkan.hpp"

class VulkanDescriptorWrapper {
private:
	VkDescriptorSetLayout* pSetLayout_;

public:
	VkDescriptorSet descriptorSet_;

	void updateImageDescriptorSet(VkDevice device, VkImageLayout layout, VkImageView view, VkSampler sampler, int dstBinding);
};
