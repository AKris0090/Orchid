#pragma once

#include "PrefilteredEnvMap.h"

class ShadowMap {
private:
	struct {
		VkImage image;
		VkDeviceMemory memory;
	} offscreen;

	VkDevice device_;
	DeviceHelper* pDevHelper_;
	uint32_t mipLevels_;
	VkFormat imageFormat_ = VK_FORMAT_D16_UNORM;

	uint32_t width_ = 1024, height_ = 1024;

	VkBuffer stagingBuffer_;
	VkDeviceMemory stagingBufferMemory_;

	VkFramebuffer sMFrameBuffer_;
	VkRenderPass sMRenderpass_;

	VkDescriptorSetLayout sMDescriptorSetLayout_;
	VkDescriptorPool sMDescriptorPool_;
	VkDescriptorSet sMDescriptorSet_;

	VkPipeline sMPipeline_;

	VkAttachmentDescription sMAttachment;
	VkDescriptorImageInfo sMImageInfo;

	VkQueue* pGraphicsQueue_;
	VkCommandPool* pCommandPool_;

	uint32_t numModels_;
	std::vector<GLTFObj*> pModels_;

	glm::vec4* lightPos;

	void findDepthFormat(VkPhysicalDevice GPU_);
	VkFormat findSupportedFormat(VkPhysicalDevice GPU_, const std::vector<VkFormat>& potentialFormats, VkImageTiling tiling, VkFormatFeatureFlags features);

	void createSMDescriptors();

	void createRenderPass();
	void createFrameBuffer();

	void createPipeline();

	uint32_t findMemoryType(VkPhysicalDevice gpu_, uint32_t typeFilter, VkMemoryPropertyFlags properties);
	VkShaderModule createShaderModule(VkDevice dev, const std::vector<char>& binary);
	std::vector<char> readFile(const std::string& filename);

	void endCommandBuffer(VkDevice device_, VkCommandBuffer cmdBuff, VkQueue* pGraphicsQueue_, VkCommandPool* pCommandPool_);

public:
	// Keep depth range as small as possible
    // for better shadow map precision
	float zNear;
	float zFar;

	VkPipelineLayout sMPipelineLayout_;


	VkImageView sMImageView_;
	VkSampler sMImageSampler_;

	struct depthMVP {
		glm::mat4 mvp;
	} depthPushBlock;

	ShadowMap(DeviceHelper* devHelper, VkQueue* graphicsQueue, VkCommandPool* cmdPool, glm::vec4* lPos, std::vector<GLTFObj*> pModels_, uint32_t numModels_);

	VkCommandBuffer render(VkCommandBuffer cmdBuf);
	void genShadowMap();
	void updateUniBuffers();
	VkDescriptorSet getDescriptorSet() { return this->sMDescriptorSet_; };
};