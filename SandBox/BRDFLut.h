#pragma once

#include "Skybox.h"

class BRDFLut {
private:
	VkDevice device_;
	DeviceHelper* pDevHelper_;
	uint32_t mipLevels_;
	VkFormat imageFormat_;

	uint32_t width_, height_;

	VkImage brdfLUTImage_;
	VkDeviceMemory brdfLUTImageMemory_;

	VkFramebuffer brdfLUTFrameBuffer_;
	VkRenderPass brdfLUTRenderpass_;

	VkDescriptorSetLayout brdfLUTDescriptorSetLayout_;
	VkDescriptorPool brdfLUTDescriptorPool_;
	VkDescriptorSet brdfLUTDescriptorSet_;

	VulkanPipelineBuilder* brdfLutPipeline_;

	void createBRDFLutImage();
	void createBRDFLutImageView();
	void createBRDFLutImageSampler();
	void createBRDFLutDescriptors();
	void createRenderPass();
	void createFrameBuffer();
	void createPipeline();
	void render();

public:
	VkImageView brdfLUTImageView_;
	VkSampler brdfLUTImageSampler_;

	BRDFLut(DeviceHelper* devHelper);
	~BRDFLut() {
		vkDestroyImage(this->device_, this->brdfLUTImage_, nullptr);
		vkDestroyFramebuffer(this->device_, this->brdfLUTFrameBuffer_, nullptr);
		vkDestroyRenderPass(this->device_, this->brdfLUTRenderpass_, nullptr);
		vkDestroyDescriptorSetLayout(this->device_, this->brdfLUTDescriptorSetLayout_, nullptr);
		vkDestroyDescriptorPool(this->device_, this->brdfLUTDescriptorPool_, nullptr);
		delete brdfLutPipeline_;
	};

	void generateBRDFLUT();
	VkDescriptorSet getDescriptorSet() { return this->brdfLUTDescriptorSet_; };
};