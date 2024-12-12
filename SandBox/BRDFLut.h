#pragma once

#include "Skybox.h"

class BRDFLut {
private:
	DeviceHelper* pDevHelper_;

	VkFramebuffer brdfLUTFrameBuffer_;
	VkRenderPass brdfLUTRenderpass_;

	VulkanDescriptorLayoutBuilder* brdfLUTDescriptorSetLayout_;
	VulkanPipelineBuilder* brdfLutPipeline_;

	void generateBRDFLUT();
	void render();
	void preDelete();

public:
	VulkanImage imageTarget_;
	VkDescriptorSet brdfLUTDescriptorSet_;

	BRDFLut(DeviceHelper* devHelper);
	~BRDFLut();
};