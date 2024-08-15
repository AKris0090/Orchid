#pragma once

#include "Skybox.h"

class BRDFLut {
private:
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

	VulkanPipelineBuilder* brdfLutPipeline_;

	void generateBRDFLUT();
	void createBRDFLutImage();
	void createBRDFLutImageView();
	void createBRDFLutImageSampler();
	void createBRDFLutDescriptors();
	void createRenderPass();
	void createFrameBuffer();
	void createPipeline();
	void render();

public:
	VkDescriptorSet brdfLUTDescriptorSet_;
	VkImageView brdfLUTImageView_;
	VkSampler brdfLUTImageSampler_;

	BRDFLut(DeviceHelper* devHelper);
	~BRDFLut();
};