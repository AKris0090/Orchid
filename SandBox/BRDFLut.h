#pragma once

#include "Skybox.h"
#include <fstream>

class BRDFLut {
private:
	VkDevice device_;
	DeviceHelper* pDevHelper_;
	uint32_t mipLevels_;
	VkFormat imageFormat_;

	uint32_t width_, height_;

	VkBuffer stagingBuffer_;
	VkDeviceMemory stagingBufferMemory_;

	VkImage brdfLUTImage_;

	VkFramebuffer brdfLUTFrameBuffer_;
	VkRenderPass brdfLUTRenderpass_;

	VkDescriptorSetLayout brdfLUTDescriptorSetLayout_;
	VkDescriptorPool brdfLUTDescriptorPool_;
	VkDescriptorSet brdfLUTDescriptorSet_;

	VkDeviceMemory brdfLUTImageMemory_;

	VkPipelineLayout brdfLUTPipelineLayout_;
	VkPipeline brdfLUTPipeline_;

	VkAttachmentDescription brdfLUTattachment;

	VkQueue* pGraphicsQueue_;
	VkCommandPool* pCommandPool_;

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

	BRDFLut(DeviceHelper* devHelper, VkQueue* graphicsQueue, VkCommandPool* cmdPool);

	void genBRDFLUT();
	VkDescriptorSet getDescriptorSet() { return this->brdfLUTDescriptorSet_; };
};