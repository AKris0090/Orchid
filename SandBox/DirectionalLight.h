#pragma once

#include "PrefilteredEnvMap.h"

class DirectionalLight {
private:
	struct {
		VkImage image;
		VkDeviceMemory memory;
	} offscreen;

	struct Cascade {
		VkFramebuffer frameBuffer;
		VkDescriptorSet descriptorSet;
		VkImageView imageView;

		float splitDepth;
		glm::mat4 viewProjectionMatrix;
	};

	VkDevice device_;
	DeviceHelper* pDevHelper_;
	uint32_t mipLevels_;
	VkFormat imageFormat_ = VK_FORMAT_D16_UNORM;

	uint32_t width_ = 1024, height_ = 1024;

	VkBuffer stagingBuffer_;
	VkDeviceMemory stagingBufferMemory_;

	VkRenderPass sMRenderpass_;

	VkDescriptorSetLayout cascadeSetLayout;
	VkDescriptorPool sMDescriptorPool_;

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

	void createSMDescriptors(glm::mat4 camProj, glm::mat4 camView, float camNear, float camFar, float aspectRatio);

	void createRenderPass();
	void createFrameBuffer();

	void createPipeline();

	uint32_t findMemoryType(VkPhysicalDevice gpu_, uint32_t typeFilter, VkMemoryPropertyFlags properties);
	VkShaderModule createShaderModule(VkDevice dev, const std::vector<char>& binary);
	std::vector<char> readFile(const std::string& filename);

	std::vector<glm::vec4> getFrustrumWorldCoordinates(const glm::mat4& proj, const glm::mat4& view);

	void endCommandBuffer(VkDevice device_, VkCommandBuffer cmdBuff, VkQueue* pGraphicsQueue_, VkCommandPool* pCommandPool_);
public:
	struct PostRenderPacket {
		VkRenderPassBeginInfo rpBeginInfo;
		VkPipeline pipline;
		VkPipelineLayout pipelineLayout;
		VkCommandBuffer commandBuffer;
	} postRenderPacket;

	// Keep depth range as small as possible
    // for better shadow map precision
	float zNear;
	float zFar;

	VkPipelineLayout sMPipelineLayout_;
	VkPipelineLayout animatedSmPipelineLayout;

	VkImageView sMImageView_;
	VkSampler sMImageSampler_;
	VkPipeline animatedSMPipeline;

	std::array<Cascade, SHADOW_MAP_CASCADE_COUNT> cascades;

	float cascadeSplitLambda;

	struct depthMVP {
		glm::mat4 model;
		uint32_t cascadeIndex;
	} depthPushBlock;

	struct UBO {
		glm::mat4 cascadeMVP[SHADOW_MAP_CASCADE_COUNT];
	};

	std::vector<VkBuffer> uniformBuffer;
	std::vector<void*> mappedBuffer;
	std::vector<VkDeviceMemory> uniformMemory;

	float swapChainWidth;
	float swapChainHeight;

	DirectionalLight(DeviceHelper* devHelper, VkQueue* graphicsQueue, VkCommandPool* cmdPool, glm::vec4* lPos, std::vector<GLTFObj*> pModels_, uint32_t numModels_, float swapChainWidth, float swapChainHeight);

	void createAnimatedPipeline(VkDescriptorSetLayout descLayout);

	PostRenderPacket render(VkCommandBuffer cmdBuf, uint32_t cascadeIndex);
	void genShadowMap(glm::mat4 camProj, glm::mat4 camView, float camNear, float camFar, float aspectRatio);
	void updateUniBuffers(glm::mat4 cameraProj, glm::mat4 camView, float cameraNearPlane, float cameraFarPlane, float aspectRatio);
	glm::mat4 getLightSpaceMatrix(float nearPlane, float farPlane, glm::mat4 camView, float aspectRatio);
};