#include "VulkanUtils.h"

void VulkanDescriptorWrapper::updateImageDescriptorSet(VkDevice device, VkImageLayout layout, VkImageView view, VkSampler sampler, int dstBinding) {
    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = layout;
    imageInfo.imageView = view;
    imageInfo.sampler = sampler;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptorSet_;
    write.dstBinding = dstBinding;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(device, 1, &write, 0, nullptr);
}