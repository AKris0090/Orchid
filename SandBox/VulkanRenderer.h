#pragma once

#include "Bloom.h"
#include "Camera.h"

#ifdef NDEBUG
const bool enableValLayers = false;
#else
const bool enableValLayers = true;
#endif

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};
const std::vector<const char*> deviceExts = {
	VK_EXT_PAGEABLE_DEVICE_LOCAL_MEMORY_EXTENSION_NAME,
	VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME,
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;

	std::optional<uint32_t> presentFamily;
	
	std::optional<uint32_t> computeFamily;

	bool isComplete() const { return (graphicsFamily.has_value() && presentFamily.has_value() && computeFamily.has_value()); }
};

struct SWChainSuppDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;

	VkSurfaceFormatKHR chooseSwSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwPresMode(const std::vector<VkPresentModeKHR>& availablePresModes);
	VkExtent2D chooseSwExtent(const VkSurfaceCapabilitiesKHR& capabilities, SDL_Window* window);
};

struct UniformBufferObject {
	glm::mat4 view;
	glm::mat4 proj;
	glm::vec4 lightPos;
	glm::vec4 viewPos;
	glm::vec4 gammaExposure;
	float cascadeSplits[4];
	glm::mat4 cascadeViewProjMat[4];
};

struct IndirectBatch {
	Material* material;
	uint32_t first;
	uint32_t count;
};

class VulkanRenderer {

private:
	PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT = nullptr;
	PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT = nullptr;

	uint32_t imageIndex_;
	bool rendered = false;
	VkSurfaceKHR surface_;

	VkSwapchainKHR swapChain_;
	std::vector<VkImage> SWChainImages_;
	VkFormat SWChainImageFormat_;
	std::vector<VkImageView> SWChainImageViews_;

	VkImage colorImage_;
	VkDeviceMemory colorImageMemory_;
	VkImageView colorImageView_;

	VkImage bloomImage_;
	VkDeviceMemory bloomImageMemory_;
	VkImageView bloomImageView_;
	
	VkImage resolveImage_;
	VkDeviceMemory resolveImageMemory_;
	VkImageView resolveImageView_;

	VkImage depthImage_;
	VkDeviceMemory depthImageMemory_;
	VkImageView depthImageView_;

	std::vector<VkFramebuffer> SWChainFrameBuffers_;
	std::vector<VkFramebuffer> depthFrameBuffers_;
	std::vector<VkFramebuffer> toneMappingFrameBuffers_;

	std::vector<VkBuffer> uniformBuffers_;
	std::vector<void*> mappedBuffers_;
	std::vector<VkDeviceMemory> uniformBuffersMemory_;

	std::vector<VkDescriptorSet> descriptorSets_;
	VkDescriptorSet computeDescriptorSet_;
	VkDescriptorSet toneMappingDescriptorSet_;
	VkDescriptorSet modelMatrixDescriptorSet_;

	std::vector<VkSemaphore> imageAcquiredSema_;
	std::vector<VkSemaphore> renderedSema_;
	std::vector<VkFence> inFlightFences_;
	VkFence computeFence;
	VkSemaphore computeSema;

	// Find the queue families given a physical device, called in isSuitable to find if the queue families support VK_QUEUE_GRAPHICS_BIT
	void loadDebugUtilsFunctions(VkDevice device);
	void updateIndividualDescriptorSet(Material& m);
	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice);
	void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
	bool checkExtSupport(VkPhysicalDevice physicalDevice);
	SWChainSuppDetails getDetails(VkPhysicalDevice physicalDevice);
	bool isSuitable(VkPhysicalDevice physicalDevice);
	VkFormat findDepthFormat();
	VkFormat findSupportedFormat(const std::vector<VkFormat>& potentialFormats, VkImageTiling tiling, VkFormatFeatureFlags features);
	void cleanupSWChain();
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void recordSkyBoxCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

public:
	int numModels_;
	int numTextures_;
	bool rotate_;
	bool frBuffResized_;
	float depthBias_;
	float maxReflectionLOD_;
	float gamma_;
	float exposure_;
	bool applyTonemap;
	float bloomRadius;
	float specularCont;
	float nDotVSpec;
	DirectionalLight* pDirectionalLight_;
	FPSCam camera_;

	VkExtent2D SWChainExtent_;

	VkBuffer vertexBuffer_;
	VkDeviceMemory vertexBufferMemory_;

	VkBuffer indexBuffer_;
	VkDeviceMemory indexBufferMemory_;

	VkBuffer screenQuadVertexBuffer;
	VkDeviceMemory screenQuadVertexBufferMemory;

	VkBuffer screenQuadIndexBuffer;
	VkDeviceMemory screenQuadIndexBufferMemory;

	VkBuffer drawCallBuffer;
	VkDeviceMemory drawCallBufferMemory;

	VkBuffer modelMatrixBuffer;
	void* mappedModelMatrixBuffer;
	VkDeviceMemory modelMatrixBufferMemory;

	std::vector<Vertex> screenQuadVertices;
	std::vector<uint32_t> screenQuadIndices;

	std::vector<Vertex> vertices_;
	std::vector<uint32_t>indices_;

	std::vector<glm::mat4> inverseBindMatrices;
	std::vector<glm::mat4> modelMatrices;

	std::vector<IndirectBatch> drawBatches;
	std::vector<VkDrawIndexedIndirectCommand> drawCommands;

	VkBuffer skinBindMatricsBuffer;
	void* mappedSkinBuffer;
	VkDeviceMemory skinBindMatricesBufferMemory;

	std::vector<GameObject*>* gameObjects;
	std::vector<AnimatedGameObject*>* animatedObjects;

	struct ComputePushConstant {
		uint32_t jointMatrixStart;
		uint32_t numVertices;
	};

	BloomHelper* bloomHelper;

	DeviceHelper* pDevHelper_;
	Skybox* pSkyBox_;
	VkCommandPool commandPool_;
	VkCommandPool computePool_;
	VkClearValue clearValue_;
	VkPhysicalDevice GPU_ = VK_NULL_HANDLE;
	VkDevice device_;
	size_t currentFrame_ = 0;
	uint32_t numMats_;
	uint32_t numImages_;
	VkInstance instance_;
	VkDebugUtilsMessengerEXT debugMessenger_;
	VulkanPipelineBuilder* opaquePipeline_;
	VulkanPipelineBuilder* prepassPipeline_;
	VulkanPipelineBuilder* toonPipeline_;
	VulkanPipelineBuilder* outlinePipeline_;
	VulkanPipelineBuilder* toneMappingPipeline_;

	VkPipelineLayout transparentPipeLineLayout_;

	VkPipeline computePipeline;
	VkPipelineLayout computePipelineLayout;

	VkPipeline computeCullPipeline;
	VkPipelineLayout computeCullPipelineLayout;

	VkPipeline transparentPipeline;

	VkRenderPass renderPass_;
	VkRenderPass depthPrepass_;
	VkRenderPass toneMapPass_;
	std::vector<VkCommandBuffer> commandBuffers_;
	VkCommandBuffer computeBuffer_;
	VkDescriptorPool descriptorPool_;
	VkQueue graphicsQueue_;
	VkQueue presentQueue_;
	VkQueue computeQueue_;
	QueueFamilyIndices QFIndices_;
	VkDescriptorSetLayout uniformDescriptorSetLayout_;
	VkDescriptorSetLayout textureDescriptorSetLayout_;
	VkDescriptorSetLayout computeDescriptorSetLayout_;
	VkDescriptorSetLayout modelMatrixSetLayout_;
	VkDescriptorSetLayout tonemappingDescriptorSetLayout_;

	VkSampler toneMappingSampler_;
	VkSampler toneMappingBloomSampler_;

	VkImage bloomResolveImage_;
	VkDeviceMemory bloomResolveImageMemory_;
	VkImageView bloomResolveImageView_;

	BRDFLut* brdfLut;
	IrradianceCube* irCube;
	PrefilteredEnvMap* prefEMap;

	VulkanRenderer();
	VkInstance createVulkanInstance(SDL_Window* window, const char* appName);
	bool checkValLayerSupport();
	void setupDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger);
	void createSurface(SDL_Window* window);
	void pickPhysicalDevice();
	void createLogicalDevice();
	void createSWChain(SDL_Window* window);
	void createImageViews();
	void createRenderPass();
	void createDescriptorSetLayout();
	void createGraphicsPipeline();
	void createDepthPipeline();
	void createOutlinePipeline();
	void createSkyBoxPipeline();
	void createToonPipeline();
	void createToneMappingPipeline();
	void createCommandPool();
	void createColorResources();
	void createDepthResources();
	void createFrameBuffer();
	void createUniformBuffers();
	void createDescriptorPool();
	void createDescriptorSets();
	void createCommandBuffers(int numFramesInFlight);
	void createSemaphores(const int maxFramesInFlight);
	void recreateSwapChain(SDL_Window* window);
	void updateUniformBuffer(uint32_t currentImage);
	void drawNewFrame(SDL_Window* window, int maxFramesInFlight);
	void postDrawEndCommandBuffer(VkCommandBuffer commandBuffer, SDL_Window* window, int maxFramesInFlight);
	void freeEverything(int framesInFlight);
	void separateDrawCalls();
	void updateModelMatrices();
	void addToDrawCalls();
	void createDrawCallBuffer();
	void createModelMatrixBuffer();
	void sortDraw(GLTFObj* obj, GLTFObj::SceneNode* node);
	void sortDraw(AnimatedGLTFObj* animObj, AnimSceneNode* node);
	void setupCompute();
	void createVertexBuffer();
	void createQuadVertexBuffer();
	void createIndexBuffer();
	void createQuadIndexBuffer();
	void updateBindMatrices();
	void updateGeneratedImageDescriptorSets();
	void renderBloom(VkCommandBuffer& commandBuffer);
	void shutdown();
};