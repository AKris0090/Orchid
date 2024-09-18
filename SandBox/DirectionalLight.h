#pragma once

#include "PrefilteredEnvMap.h"
#include "Camera.h"

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

	std::vector<float> shadowCascadeLevels{};

	DeviceHelper* pDevHelper_;
	uint32_t mipLevels_;
	VkFormat imageFormat_ = VK_FORMAT_D16_UNORM;

	uint32_t width_, height_;

	VkBuffer stagingBuffer_;
	VkDeviceMemory stagingBufferMemory_;

	VulkanDescriptorLayoutBuilder* cascadeSetLayout;
	VkDescriptorPool sMDescriptorPool_;

	VkAttachmentDescription sMAttachment;
	VkDescriptorImageInfo sMImageInfo;

	VkQueue* pGraphicsQueue_;
	VkCommandPool* pCommandPool_;

	void findDepthFormat(VkPhysicalDevice GPU_);
	VkFormat findSupportedFormat(VkPhysicalDevice GPU_);

	void createSMDescriptors(FPSCam* camera, int framesInFlight);

	void createRenderPass();
	void createFrameBuffer(int framesInFlight);

	uint32_t findMemoryType(VkPhysicalDevice gpu_, uint32_t typeFilter, VkMemoryPropertyFlags properties);
	VkShaderModule createShaderModule(VkDevice dev, const std::vector<char>& binary);
	std::vector<char> readFile(const std::string& filename);

	std::vector<glm::vec4> getFrustrumWorldCoordinates(const glm::mat4& proj, const glm::mat4& view);
public:
	struct PostRenderPacket {
		VkRenderPassBeginInfo rpBeginInfo;
		VkPipeline pipline;
		VkPipelineLayout pipelineLayout;
		VkCommandBuffer commandBuffer;
	} postRenderPacket;
	VkRenderPass sMRenderpass_;

	Transform transform;

	VulkanPipelineBuilder* sMPipeline_;

	VkImageView sMImageView_;
	VkSampler sMImageSampler_;

	std::vector<std::array<Cascade, SHADOW_MAP_CASCADE_COUNT>> cascades;

	float cascadeSplitLambda;

	struct depthMVP {
		glm::mat4 model;
		uint32_t cascadeIndex;
	} depthPushBlock;

	struct UBO {
		glm::mat4 cascadeMVPUniform[SHADOW_MAP_CASCADE_COUNT];
	};

	std::vector<VkBuffer> uniformBuffer;
	std::vector<void*> mappedBuffer;
	std::vector<VkDeviceMemory> uniformMemory;

	float swapChainWidth;
	float swapChainHeight;

	DirectionalLight(glm::vec3 lPos);

	void setup(DeviceHelper* devHelper, VkQueue* graphicsQueue, VkCommandPool* cmdPool, float swapChainWidth, float swapChainHeight);
	PostRenderPacket render(VkCommandBuffer cmdBuf, uint32_t cascadeIndex, int currentFrame);
	void genShadowMap(FPSCam* camera, VkDescriptorSetLayout* modelMatrixDescriptorSet, int framesInFlight);
	void updateUniBuffers(FPSCam* camera, int currentFrame);
	void createPipeline(VulkanDescriptorLayoutBuilder* modelMatrixDescriptorSet);
};