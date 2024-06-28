#pragma once

#include "SDL2/SDL.h"
#include "SDL2/SDL_vulkan.h"
#include <cstring>
#include <optional>
#include <set>
#include <string>
#include <algorithm>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

#include "GameObject.h"
#include "AnimatedGameObject.h"
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
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

// Queue family struct
struct QueueFamilyIndices {
	// Graphics families initialization
	std::optional<uint32_t> graphicsFamily;

	// Present families initialization as well
	std::optional<uint32_t> presentFamily;

	// General check to make things a bit more conveneient
	bool isComplete() { return (graphicsFamily.has_value() && presentFamily.has_value()); }
};

// Swap chain support details struct - holds information to create the swapchain
struct SWChainSuppDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;

	VkSurfaceFormatKHR chooseSwSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR chooseSwPresMode(const std::vector<VkPresentModeKHR>& availablePresModes);
	VkExtent2D chooseSwExtent(const VkSurfaceCapabilitiesKHR& capabilities, SDL_Window* window);
};

class VulkanRenderer {

private:
	PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT = nullptr;
	PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT = nullptr;

	uint32_t imageIndex_;

	// SDL Surface handle
	VkSurfaceKHR surface_;

	// Swap chain handles
	VkSwapchainKHR swapChain_;
	std::vector<VkImage> SWChainImages_;
	VkFormat SWChainImageFormat_;
	std::vector<VkImageView> SWChainImageViews_;

	// Color image and mem handles
	VkImage colorImage_;
	VkDeviceMemory colorImageMemory_;
	VkImageView colorImageView_;
	// Depth image and mem handles
	VkImage depthImage_;
	VkDeviceMemory depthImageMemory_;
	VkImageView depthImageView_;
	// Handle to hold the frame buffers
	std::vector<VkFramebuffer> SWChainFrameBuffers_;
	std::vector<VkFramebuffer> skyBoxFrameBuffers_;
	// Uniform buffers handle
	std::vector<VkBuffer> uniformBuffers_;
	std::vector<void*> mappedBuffers_;
	std::vector<VkDeviceMemory> uniformBuffersMemory_;
	// Descriptor set handles
	std::vector<VkDescriptorSet> descriptorSets_;
	// Handles for the two semaphores - one for if the image is available and the other to present the image
	std::vector<VkSemaphore> imageAcquiredSema_;
	std::vector<VkSemaphore> renderedSema_;
	// Handle for the in-flight-fence
	std::vector<VkFence> inFlightFences_;
	std::vector<VkFence> imagesInFlight_;

	bool rendered = false;

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
	static std::vector<char> readFile(const std::string& fileName);
	VkShaderModule createShaderModule(const std::vector<char>& binary);
	void cleanupSWChain();
	void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
	void recordSkyBoxCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

public:
	int numModels_;
	int numTextures_;
	bool rotate_ = false;
	bool frBuffResized_ = false;
	DirectionalLight* pDirectionalLight;
	FPSCam camera_;
	float depthBias;
	float maxReflectionLOD;
	VkExtent2D SWChainExtent_;

	std::vector<Vertex> vertices_;
	std::vector<uint32_t>indices_;

	VkBuffer vertexBuffer_;
	VkDeviceMemory vertexBufferMemory_;

	VkBuffer indexBuffer_;
	VkDeviceMemory indexBufferMemory_;

	std::vector<GameObject*>* gameObjects;
	std::vector<AnimatedGameObject*>* animatedObjects;

	DeviceHelper* pDevHelper_;
	Skybox* pSkyBox_;
	VkCommandPool commandPool_;
	VkClearValue clearValue_;
	VkPhysicalDevice GPU_ = VK_NULL_HANDLE;
	VkDevice device_;
	size_t currentFrame_ = 0;
	uint32_t numMats_;
	uint32_t numImages_;
	// Handles for all variables, made public so they can be accessed by main and destroys
	VkInstance instance_;
	VkDebugUtilsMessengerEXT debugMessenger_;
	// Pipeline Layout for "gloabls" to change shaders
	VkPipelineLayout opaquePipeLineLayout_;
	VkPipelineLayout transparentPipeLineLayout_;
	VkPipelineLayout opaqueAnimatedPipelineLayout_;
	VkPipelineLayout transparentAnimatedPipelineLayout_;

	// OPAQUE AND TRANSPARENT PIPELINES
	VkPipeline opaquePipeline;
	VkPipeline transparentPipeline;
	VkPipeline animatedOpaquePipeline;
	VkPipeline animatedTransparentPipeline;

	// Render pass handles
	VkRenderPass renderPass_;
	VkRenderPass skyboxRenderPass_;
	// Handle for the list of command buffers
	std::vector<VkCommandBuffer> commandBuffers_;
	// Descriptor pool handles
	VkDescriptorPool descriptorPool_;
	// Device and queue handles
	VkQueue graphicsQueue_;
	VkQueue presentQueue_;
	QueueFamilyIndices QFIndices_;
	VkSampleCountFlagBits msaaSamples_ = VK_SAMPLE_COUNT_8_BIT;
	// Descriptor Set Layout Handle
	VkDescriptorSetLayout uniformDescriptorSetLayout_;
	VkDescriptorSetLayout textureDescriptorSetLayout_;
	VkDescriptorSetLayout animatedDescriptorSetLayout_;
	BRDFLut* brdfLut;
	IrradianceCube* irCube;
	PrefilteredEnvMap* prefEMap;

	VulkanRenderer();
	// Create the vulkan instance
	VkInstance createVulkanInstance(SDL_Window* window, const char* appName);
	// Check if the validation layers requested are supported
	bool checkValLayerSupport();
	// Debug messenger methods - create, populate, and destroy the debug messenger
	void setupDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger);
	// Create the SDL surface using Vulkan
	void createSurface(SDL_Window* window);
	// Poll through the available physical devices and choose one using isSuitable()
	void pickPhysicalDevice();
	// Using the physical device, create the logical device
	void createLogicalDevice();
	// Initialize the swap chain
	void createSWChain(SDL_Window* window);
	// With the swap chain, create the image views
	void createImageViews();
	// Create the render pass
	void createRenderPass();
	// Create the descriptor set layout
	void createDescriptorSetLayout();
	// Create the graphics pipeline
	void createGraphicsPipeline();
	void createAnimatedGraphicsPipeline();
	void createSkyBoxPipeline();
	// You have to first record all the operations to perform, so we need a command pool
	void createCommandPool();
	// Color image function
	void createColorResources();
	// Create depth image function
	void createDepthResources();
	// Creating the all-important frame buffer
	void createFrameBuffer();
	// Creation of uniform buffers
	void createUniformBuffers();
	// Descriptor Pool creation
	void createDescriptorPool();
	// Descriptor Set Creations
	void createDescriptorSets();
	// Create a list of command buffer objects
	void createCommandBuffers(int numFramesInFlight);
	// Create the semaphores, signaling objects to allow asynchronous tasks to happen at the same time
	void createSemaphores(const int maxFramesInFlight);
	void recreateSwapChain(SDL_Window* window);
	// Display methods - uniform buffer and fetching new frames
	void updateUniformBuffer(uint32_t currentImage);
	void drawNewFrame(SDL_Window* window, int maxFramesInFlight);
	void postDrawEndCommandBuffer(VkCommandBuffer commandBuffer, SDL_Window* window, int maxFramesInFlight);
	void freeEverything(int framesInFlight);
	void separateDrawCalls();
	void sortDraw(GLTFObj* obj, GLTFObj::SceneNode* node);
	void sortDraw(AnimatedGLTFObj* animObj, AnimatedGLTFObj::SceneNode* node);

	void createVertexBuffer();
	void createIndexBuffer();

	void updateGeneratedImageDescriptorSets();

	struct UniformBufferObject {
		glm::mat4 view;
		glm::mat4 proj;
		glm::vec4 lightPos;
		glm::vec4 viewPos;
		float cascadeSplits[4];
		glm::mat4 cascadeViewProjMat[4];
	};
};