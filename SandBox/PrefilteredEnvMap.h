#pragma once
#pragma once

#include "IrradianceCube.h"
#include <fstream>

class PrefilteredEnvMap{
private:
	Skybox* pSkybox_;

	struct {
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

	VkBuffer stagingBuffer_;
	VkDeviceMemory stagingBufferMemory_;

	VkImage prefEMapImage_;

	VkFramebuffer prefEMapFrameBuffer_;
	VkRenderPass prefEMapRenderpass_;

	VkDescriptorSetLayout prefEMapDescriptorSetLayout_;
	VkDescriptorPool prefEMapDescriptorPool_;
	VkDescriptorSet prefEMapDescriptorSet_;

	VkDeviceMemory prefEMapImageMemory_;

	VulkanPipelineBuilder* prefEMPipeline_;

	VkAttachmentDescription prefEMapattachment;
	VkDescriptorImageInfo prefevImageInfo;

	VkQueue* pGraphicsQueue_;
	VkCommandPool* pCommandPool_;

	void createprefEMapImage();
	void createprefEMapImageView();
	void createprefEMapImageSampler();

	void createprefEMapDescriptors();

	void createRenderPass();
	void createFrameBuffer();
	void render(VkBuffer& vertexBuffer, VkBuffer& indexBuffer);
	void createPipeline();

	std::vector<char> readFile(const std::string& filename);
	VkShaderModule createShaderModule(VkDevice dev, const std::vector<char>& binary);
	uint32_t findMemoryType(VkPhysicalDevice gpu_, uint32_t typeFilter, VkMemoryPropertyFlags properties);
	void transitionImageLayout(VkCommandBuffer cmdBuff, VkImageSubresourceRange subresourceRange, VkImageLayout oldLayout, VkImageLayout newLayout, VkImage prefImage);

	void endCommandBuffer(VkDevice device_, VkCommandBuffer cmdBuff, VkQueue* pGraphicsQueue_, VkCommandPool* pCommandPool_);

public:
	VkImageView prefEMapImageView_;
	VkSampler prefEMapImageSampler_;

	PrefilteredEnvMap(DeviceHelper* devHelper, VkQueue* graphicsQueue, VkCommandPool* cmdPool, Skybox* pSkybox);
	~PrefilteredEnvMap() {
		vkDestroyImage(this->pDevHelper_->device_, this->prefEMapImage_, nullptr);
		vkDestroyFramebuffer(this->pDevHelper_->device_, this->prefEMapFrameBuffer_, nullptr);
		vkDestroyRenderPass(this->pDevHelper_->device_, this->prefEMapRenderpass_, nullptr);
		vkDestroyDescriptorSetLayout(this->pDevHelper_->device_, this->prefEMapDescriptorSetLayout_, nullptr);
		vkDestroyDescriptorPool(this->pDevHelper_->device_, this->prefEMapDescriptorPool_, nullptr);
		delete prefEMPipeline_;
	};

	void genprefEMap(VkBuffer& vertexBuffer, VkBuffer& indexBuffer);
	VkDescriptorSet getDescriptorSet() { return this->prefEMapDescriptorSet_; };
};