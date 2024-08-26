#pragma once

#include "IrradianceCube.h"

class PrefilteredEnvMap{
private:
	struct OffscreenStruct{
		VkImage image;
		VkImageView view;
		VkDeviceMemory memory;
		VkFramebuffer framebuffer;
	} offscreen;

	struct PushBlock {
		glm::mat4 mvp;
		float roughness;
		uint32_t numSamples = 32u;
	} pushBlock;

	DeviceHelper* pDevHelper_;
	uint32_t mipLevels_;
	VkFormat imageFormat_;
	uint32_t width_, height_;
	Skybox* pSkybox_;

	VkImage prefEMapImage_;
	VkFramebuffer prefEMapFrameBuffer_;
	VkRenderPass prefEMapRenderpass_;
	VkDescriptorSetLayout prefEMapDescriptorSetLayout_;
	VkDescriptorPool prefEMapDescriptorPool_;
	VkDeviceMemory prefEMapImageMemory_;
	VkAttachmentDescription prefEMapattachment;
	VkDescriptorImageInfo prefevImageInfo;

	VulkanPipelineBuilder* prefEMPipeline_;

	void createprefEMapImage();
	void createprefEMapImageView();
	void createprefEMapImageSampler();
	void createprefEMapDescriptors();
	void createRenderPass();
	void createFrameBuffer();
	void createPipeline();
	void render(VkBuffer& vertexBuffer, VkBuffer& indexBuffer);
	void genprefEMap(VkBuffer& vertexBuffer, VkBuffer& indexBuffer);

public:
	VkImageView prefEMapImageView_;
	VkSampler prefEMapImageSampler_;
	VkDescriptorSet prefEMapDescriptorSet_;

	PrefilteredEnvMap(DeviceHelper* devHelper, Skybox* pSkybox, VkBuffer& vertexBuffer, VkBuffer& indexBuffer);
	~PrefilteredEnvMap();
};