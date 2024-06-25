#pragma once

#include "PrefilteredEnvMap.h"
#include "Camera.h"

class DirectionalLight {
private:
	VkBuffer indirectCommandsBuffer;
	VkDeviceMemory indirectCommandsBufferMemory;

	VkBuffer indirectDrawCountBuffer;
	VkDeviceMemory indirectDrawCountBufferMemory;

	VkBuffer instanceDataBuffer;
	VkDeviceMemory instanceDataBufferMemory;

	VkBuffer pcBlockBuffer;
	VkDeviceMemory pcBlockBufferMemory;

	VkBuffer pushConstantBuffer;
	void* mappedPushConstantBuffer;
	VkDeviceMemory pushConstantBufferMemory;

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

	VkQueue computeQueue_;
	VkCommandPool computeCommandPool_;
	VkCommandBuffer computeCommandBuffer_;
	VkFence computeFence_;
	VkSemaphore computeSemaphore_;
	VkDescriptorSetLayout computeDescriptorSetLayout_;
	VkDescriptorSet computeDescriptorSet_;
	VkPipelineLayout computePipelineLayout_;
	VkPipeline computePipeline_;

	VkDevice device_;
	DeviceHelper* pDevHelper_;
	uint32_t mipLevels_;
	VkFormat imageFormat_ = VK_FORMAT_D16_UNORM;

	uint32_t width_, height_;

	VkBuffer stagingBuffer_;
	VkDeviceMemory stagingBufferMemory_;

	VkRenderPass sMRenderpass_;

	VkDescriptorSetLayout cascadeSetLayout;
	VkDescriptorPool sMDescriptorPool_;

	VkAttachmentDescription sMAttachment;
	VkDescriptorImageInfo sMImageInfo;

	VkQueue* pGraphicsQueue_;
	VkCommandPool* pCommandPool_;

	void findDepthFormat(VkPhysicalDevice GPU_);
	VkFormat findSupportedFormat(VkPhysicalDevice GPU_, const std::vector<VkFormat>& potentialFormats, VkImageTiling tiling, VkFormatFeatureFlags features);

	void createSMDescriptors(FPSCam* camera);

	void createRenderPass();
	void createFrameBuffer();

	void createPipeline();
	void createInstanceBuffers();
	uint32_t findMemoryType(VkPhysicalDevice gpu_, uint32_t typeFilter, VkMemoryPropertyFlags properties);
	VkShaderModule createShaderModule(VkDevice dev, const std::vector<char>& binary);
	std::vector<char> readFile(const std::string& filename);
public:
	uint32_t numStaticShadowDrawCalls = 0;
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

	Transform transform;
	VkPipeline sMPipeline_;
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

	struct shadowInstanceData {
		std::array<glm::vec4, 2> aabbExtent;
	};
	
	struct {
		uint32_t drawCount;
	} indirectStats;

	std::vector<VkDrawIndexedIndirectCommand> drawIndexedIndirectCommands;
	std::vector<shadowInstanceData> shadowCallData;
	std::vector<depthMVP> pCBlockData;

	std::vector<MeshHelper::Vertex> shadowVertices;
	std::vector<uint32_t> shadowIndices;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;

	void createVertexBuffer();
	void createIndexBuffer();

	struct UBO {
		glm::mat4 cascadeMVP[SHADOW_MAP_CASCADE_COUNT];
	};

	std::vector<VkBuffer> uniformBuffer;
	std::vector<void*> mappedBuffer;
	std::vector<VkDeviceMemory> uniformMemory;

	float swapChainWidth;
	float swapChainHeight;

	DirectionalLight(glm::vec3 lPos);

	void setup(DeviceHelper* devHelper, VkQueue* graphicsQueue, VkCommandPool* cmdPool, float swapChainWidth, float swapChainHeight);
	void createAnimatedPipeline(VkDescriptorSetLayout descLayout);
	PostRenderPacket render(VkCommandBuffer cmdBuf, uint32_t cascadeIndex);
	void genShadowMap(FPSCam* camera);
	void updateUniBuffers(FPSCam* camera);
	
	void setupCompute();
	void updateComputeBuffers(int cascadeIndex);
	void executeCompute(VkCommandBuffer commandBuffer, int cascadeIndex);
};