#pragma once

#include "BRDFLut.h"

class IrradianceCube {
private:
	struct OffscreenStruct {
		VkImage image;
		VkDeviceMemory memory;
		VkImageView view;
		VkFramebuffer framebuffer;
	} offscreen;

	struct PushBlock {
		glm::mat4 mvp = glm::mat4(1.0f);
		float deltaPhi = (2.0f * float(PI)) / 180.0f;
		float deltaTheta = (0.5f * float(PI)) / 64.0f;
	} pushBlock;

	DeviceHelper* pDevHelper_;
	uint32_t mipLevels_;
	VkFormat imageFormat_;
	uint32_t width_, height_;
	Skybox* pSkybox_;

	VkImage iRCubeImage_;
	VkFramebuffer iRCubeFrameBuffer_;
	VkRenderPass iRCubeRenderpass_;
	VkDescriptorPool iRCubeDescriptorPool_;
	VkDeviceMemory iRCubeImageMemory_;
	VkAttachmentDescription iRCubeattachment;
	VkDescriptorImageInfo irImageInfo;

	VulkanDescriptorLayoutBuilder* iRCubeDescriptorSetLayout_;
	VulkanPipelineBuilder* iRPipeline_;

	void geniRCube(VkBuffer& vertexBuffer, VkBuffer& indexBuffer);
	void createiRCubeImage();
	void createiRCubeImageView();
	void createiRCubeImageSampler();
	void createiRCubeDescriptors();
	void createRenderPass();
	void createFrameBuffer();
	void createPipeline();
	void render(VkBuffer& vertexBuffer, VkBuffer& indexBuffer);
	void preDelete();

public:
	VkImageView iRCubeImageView_;
	VkSampler iRCubeImageSampler_;
	VkDescriptorSet iRCubeDescriptorSet_;

	IrradianceCube(DeviceHelper* devHelper, Skybox* pSkybox, VkBuffer& vertexBuffer, VkBuffer& indexBuffer);
	~IrradianceCube();
};