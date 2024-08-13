#pragma once
#pragma once

#include "BRDFLut.h"
#include <fstream>

class IrradianceCube {
private:
	struct {
		VkImage image;
		VkDeviceMemory memory;
		VkImageView view;
		VkFramebuffer framebuffer;
	} offscreen;

	struct PushBlock {
		glm::mat4 mvp;
		float deltaPhi = (2.0f * float(PI)) / 180.0f;
		float deltaTheta = (0.5f * float(PI)) / 64.0f;
	} pushBlock;

	DeviceHelper* pDevHelper_;
	uint32_t mipLevels_;
	VkFormat imageFormat_;
	Skybox* pSkybox_;

	uint32_t width_, height_;

	VkBuffer stagingBuffer_;
	VkDeviceMemory stagingBufferMemory_;

	VkImage iRCubeImage_;

	VkFramebuffer iRCubeFrameBuffer_;
	VkRenderPass iRCubeRenderpass_;

	VkDescriptorSetLayout iRCubeDescriptorSetLayout_;
	VkDescriptorPool iRCubeDescriptorPool_;
	VkDescriptorSet iRCubeDescriptorSet_;

	VkDeviceMemory iRCubeImageMemory_;

	VulkanPipelineBuilder* iRPipeline_;

	VkAttachmentDescription iRCubeattachment;
	VkDescriptorImageInfo irImageInfo;

	VkQueue* pGraphicsQueue_;
	VkCommandPool* pCommandPool_;

	void createiRCubeImage();
	void createiRCubeImageView();
	void createiRCubeImageSampler();

	void createiRCubeDescriptors();

	void createRenderPass();
	void createFrameBuffer();
	void render(VkBuffer& vertexBuffer, VkBuffer& indexBuffer);
	void createPipeline();

	VkShaderModule createShaderModule(VkDevice dev, const std::vector<char>& binary);
	void transitionImageLayout(VkCommandBuffer cmdBuff, VkImageSubresourceRange subresourceRange, VkImageLayout oldLayout, VkImageLayout newLayout, VkImage prefImage);

	void endCommandBuffer(VkDevice device_, VkCommandBuffer cmdBuff, VkQueue* pGraphicsQueue_, VkCommandPool* pCommandPool_);

public:
	VkImageView iRCubeImageView_;
	VkSampler iRCubeImageSampler_;

	IrradianceCube(DeviceHelper* devHelper, VkQueue* graphicsQueue, VkCommandPool* cmdPool, Skybox* pSkybox);
	~IrradianceCube() {
		vkDestroyImage(this->pDevHelper_->device_, this->iRCubeImage_, nullptr);
		vkDestroyFramebuffer(this->pDevHelper_->device_, this->iRCubeFrameBuffer_, nullptr);
		vkDestroyRenderPass(this->pDevHelper_->device_, this->iRCubeRenderpass_, nullptr);
		vkDestroyDescriptorSetLayout(this->pDevHelper_->device_, this->iRCubeDescriptorSetLayout_, nullptr);
		vkDestroyDescriptorPool(this->pDevHelper_->device_, this->iRCubeDescriptorPool_, nullptr);
		delete iRPipeline_;
	};

	void geniRCube(VkBuffer& vertexBuffer, VkBuffer& indexBuffer);
	VkDescriptorSet getDescriptorSet() { return this->iRCubeDescriptorSet_; };
};