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
	VkBuffer stagingBuffer_;
	VkDeviceMemory stagingBufferMemory_;

	VkFramebuffer brdfLUTFrameBuffer_;
	VkRenderPass brdfLUTRenderpass_;

	VkDescriptorSetLayout brdfLUTDescriptorSetLayout_;
	VkDescriptorPool brdfLUTDescriptorPool_;
	VkDescriptorSet brdfLUTDescriptorSet_;

	VkPipelineLayout brdfLUTPipelineLayout_;
	VkPipeline brdfLUTPipeline_;

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

	void genBRDFLUT();
	VkDescriptorSet getDescriptorSet() { return this->brdfLUTDescriptorSet_; };
};