#include "VulkanRenderer.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#define TINYGLTF_IMPLEMENTATION

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT

#include <tiny_gltf.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
TEXTURE HELPER CLASS
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class TextureHelper {
private:
    uint32_t mipLevels = VK_SAMPLE_COUNT_1_BIT;

    // Helpers
    std::string texPath;

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);

    void createTextureImages();
    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

public:
    // Texture image and mem handles
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;

    VkSampler textureSampler;
    VkDescriptorSet descriptorSet;
    VulkanRenderer* vkR;
    tinygltf::Model* in;
    VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
    int i;
    TextureHelper(tinygltf::Model& in, int i);

    void createTextureImageView(VkFormat f = VK_FORMAT_R8G8B8A8_SRGB);
    void createTextureImageSampler();

    void load();
    void free();
    void destroy();
};


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
PUBLIC ADT METHODS
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

VulkanRenderer::VulkanRenderer(int numModels, int numTextures) {
    this->numModels = numModels;
    this->numTextures = numTextures;
}

void VulkanRenderer::updateUniformBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    // Camera logic
    this->camera.update();

    UniformBufferObject ubo{};
    ubo.view = this->camera.getViewMatrix();
    ubo.proj = glm::perspective(glm::radians(70.0f), this->SWChainExtent.width / (float)this->SWChainExtent.height, 0.0001f, 10000.0f);
    ubo.viewPos = this->camera.viewPos;
    ubo.lightPos = *(this->lightPos);

    ubo.proj[1][1] *= -1;

    void* data;
    vkMapMemory(this->device, this->uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(this->device, this->uniformBuffersMemory[currentImage]);
}

void VulkanRenderer::drawNewFrame(SDL_Window * window, int maxFramesInFlight) {
    // Wait for the frame to be finished, with the fences
    vkWaitForFences(this->device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    // Acquire an image from the swap chain, execute the command buffer with the image attached in the framebuffer, and return to swap chain as ready to present
    // Disable the timeout with UINT64_MAX
    VkResult res1 = vkAcquireNextImageKHR(this->device, this->swapChain, UINT64_MAX, this->imageAcquiredSema[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (res1 == VK_ERROR_OUT_OF_DATE_KHR) {
        this->recreateSwapChain(window);
        return;
    }
    else if (res1 != VK_SUCCESS && res1 != VK_SUBOPTIMAL_KHR) {
        std::cout << "no swap chain image, bum!" << std::endl;
        std::_Xruntime_error("Failed to acquire a swap chain image!");
    }

    updateUniformBuffer(imageIndex);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
CREATE SDL SURFACE
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderer::createSurface(SDL_Window* window) {
    SDL_Vulkan_CreateSurface(window, instance, &surface);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
DEBUG AND DEBUG MESSENGER METHODS
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
}

// Create the debug messenger using createInfo struct
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

// Fill the debug messenger with information to call back
void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

// Debug messenger creation and population called here
void VulkanRenderer::setupDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger) {
    if (enableValLayers) {

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }
    else {
        return;
    }
}

// After debug messenger is used, destroy
void VulkanRenderer::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
VALIDATION LAYER SUPPORT
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Check if validation layers requested are supported by querying the available layers
bool VulkanRenderer::checkValLayerSupport() {
    uint32_t numLayers;
    vkEnumerateInstanceLayerProperties(&numLayers, nullptr);

    std::vector<VkLayerProperties> available(numLayers);
    vkEnumerateInstanceLayerProperties(&numLayers, available.data());

    for (const char* name : validationLayers) {
        bool found = false;

        // Check if layer is found using strcmp
        for (const auto& layerProps : available) {
            if (strcmp(name, layerProps.layerName) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            return false;
        }
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
CREATE VULKAN INSTANCE
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

VkInstance VulkanRenderer::createVulkanInstance(SDL_Window* window, const char* appName) {
    if ((enableValLayers == true) && (checkValLayerSupport() == false)) {
        std::_Xruntime_error("Validation layers were requested, but none were available");
    }

    // Get application information for the create info struct
    VkApplicationInfo aInfo{};
    aInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    aInfo.pApplicationName = appName;
    aInfo.applicationVersion = 1;
    aInfo.pEngineName = "No Engine";
    aInfo.engineVersion = 1;
    aInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo instanceCInfo{};
    instanceCInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCInfo.pApplicationInfo = &aInfo;

    // Poll extensions and add to create info struct
    unsigned int extensionCount = 0;
    if (!SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr)) {
        std::_Xruntime_error("Unable to figure out the number of vulkan extensions!");
    }

    std::vector<const char*> extNames(extensionCount);
    if (!SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extNames.data())) {
        std::_Xruntime_error("Unable to figure out the vulkan extension names!");
    }

    // Add the validation layers extension to the extension names array
    if (enableValLayers) {
        extNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    // Fill create info struct with extension count and name information
    instanceCInfo.enabledExtensionCount = static_cast<uint32_t>(extNames.size());
    instanceCInfo.ppEnabledExtensionNames = extNames.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    // If using validation layers, add information to the create info struct
    if (enableValLayers) {
        instanceCInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        instanceCInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo(debugCreateInfo);
        instanceCInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
    }
    else {
        instanceCInfo.enabledLayerCount = 0;
    }

    // Create the instance
    if (vkCreateInstance(&instanceCInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }

    clearValue.color = { {0.0f, 0.0f, 0.0f, 1.0f} };

    return instance;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
CHECKING EXENSION SUPPORT
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool VulkanRenderer::checkExtSupport(VkPhysicalDevice physicalDevice) {
    // Poll available extensions provided by the physical device and then check if required extensions are among them
    uint32_t numExts;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &numExts, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(numExts);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &numExts, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExts.begin(), deviceExts.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    // Return true or false depending on if the required extensions are fulfilled
    return requiredExtensions.empty();
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
SWAP CHAIN METHODS
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// First aspect : image format
VkSurfaceFormatKHR SWChainSuppDetails::chooseSwSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for (const auto& availableFormat : availableFormats) {
        // If it has the right color space and format, return it. UNORM for color mode of imgui
        if (availableFormat.format == VK_FORMAT_R8G8B8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    // Otherwise, return the first specified format
    return availableFormats[0];
}

// Second aspect : present mode
VkPresentModeKHR SWChainSuppDetails::chooseSwPresMode(const std::vector<VkPresentModeKHR>& availablePresModes) {
    // Using Mailbox present mode, if possible
    // Can also use: VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIF_RELAXED_KHR, or VK_PRESENT_MODE_MAILBOX_KHR
    for (const VkPresentModeKHR& availablePresMode : availablePresModes) {
        if (availablePresMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresMode;
        }
    }

    // If device does not support it, use First-In-First-Out present mode
    return VK_PRESENT_MODE_FIFO_KHR;
}

// Third aspect : image extent
VkExtent2D SWChainSuppDetails::chooseSwExtent(const VkSurfaceCapabilitiesKHR& capabilities, SDL_Window* window) {
    if (capabilities.currentExtent.width != UINT32_MAX) {
        return capabilities.currentExtent;
    }
    else {
        int width, height;
        SDL_GL_GetDrawableSize(window, &width, &height);

        VkExtent2D actExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actExtent.width = std::clamp(actExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actExtent.height = std::clamp(actExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

        return actExtent;
    }
}

// Get details of the capabilities, formats, and presentation modes avialable from the physical device
SWChainSuppDetails VulkanRenderer::getDetails(VkPhysicalDevice physicalDevice) {
    SWChainSuppDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

QueueFamilyIndices VulkanRenderer::findQueueFamilies(VkPhysicalDevice physicalDevice) {
    QueueFamilyIndices indices;

    // Get queue families and store in queueFamilies array
    uint32_t numQueueFamilies = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &numQueueFamilies, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(numQueueFamilies);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &numQueueFamilies, queueFamilies.data());

    // Find at least 1 queue family that supports VK_QUEUE_GRAPHICS_BIT
    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 prSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &prSupport);

        if (prSupport) {
            indices.presentFamily = i;
        }

        // Early exit
        if (indices.isComplete()) {
            break;
        }

        i++;
    }

    return indices;
}

// Actual creation of the swap chain
void VulkanRenderer::createSWChain(SDL_Window* window) {
    SWChainSuppDetails swInfo = getDetails(GPU);

    VkSurfaceFormatKHR surfaceFormat = swInfo.chooseSwSurfaceFormat(swInfo.formats);
    VkPresentModeKHR presentMode = swInfo.chooseSwPresMode(swInfo.presentModes);
    VkExtent2D extent = swInfo.chooseSwExtent(swInfo.capabilities, window);

    // Also set the number of images we want in the swap chain, accomodate for driver to complete internal operations before we can acquire another image to render to
    uint32_t numImages = swInfo.capabilities.minImageCount + 1;

    // Make sure to not exceed maximum number of images: 0 means no maximum
    if (swInfo.capabilities.maxImageCount > 0 && numImages > swInfo.capabilities.maxImageCount) {
        numImages = swInfo.capabilities.maxImageCount;
    }

    // Fill in the structure to create the swap chain object
    VkSwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = surface;

    // Details of swap chain images
    swapchainCreateInfo.minImageCount = numImages;
    swapchainCreateInfo.imageFormat = surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent = extent;
    // Unless developing a stereoscopic 3D image, this is always 1
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    // Specify how images from swap chain are handled
    // If graphics queue family is different from the present queue family, draw on the images in swap chain from graphics and submit them on present
    QueueFamilyIndices indices = findQueueFamilies(GPU);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    // For transforming the image (rotation, translation, etc.)
    swapchainCreateInfo.preTransform = swInfo.capabilities.currentTransform;

    // Alpha channel is used for blending with other windows, so ignore
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    // Unless you want to be able to read pixels that are obscured because of another window in front of them, set clipped to VK_TRUE
    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;

    VkResult res = vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapChain);
    if (res != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device, swapChain, &numImages, nullptr);
    SWChainImages.resize(numImages);
    vkGetSwapchainImagesKHR(device, swapChain, &numImages, SWChainImages.data());

    SWChainImageFormat = surfaceFormat.format;
    SWChainExtent = extent;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
PHYSICAL DEVICE AND LOGICAL DEVICE SELECTION AND CREATION METHODS
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool VulkanRenderer::isSuitable(VkPhysicalDevice physicalDevice) {
    // If specific feature is needed, then poll for it, otherwise just return true for any suitable Vulkan supported GPU
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

    // Make sure the extensions are supported using the method defined above
    bool extsSupported = checkExtSupport(physicalDevice);

    // Make sure the physical device is capable of supporting a swap chain with the right formats and presentation modes
    bool isSWChainAdequate = false;
    if (extsSupported) {
        SWChainSuppDetails SWChainSupp = getDetails(physicalDevice);
        isSWChainAdequate = !SWChainSupp.formats.empty() && !SWChainSupp.presentModes.empty();
    }

    VkPhysicalDeviceFeatures supportedFeatures;
    vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

    return (indices.isComplete() && extsSupported && isSWChainAdequate && supportedFeatures.samplerAnisotropy);
}

VkSampleCountFlagBits getMaxUsableSampleCount(VkPhysicalDevice dev) {
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(dev, &physicalDeviceProperties);

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
    if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
    if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

    return VK_SAMPLE_COUNT_1_BIT;
}

// Parse through the list of available physical devices and choose the one that is suitable
void VulkanRenderer::pickPhysicalDevice() {
    // Enumerate physical devices and store it in a variable, and check if there are none available
    uint32_t numDevices = 0;
    vkEnumeratePhysicalDevices(instance, &numDevices, nullptr);
    if (numDevices == 0) {
        std::cout << "no GPUs found with Vulkan support!" << std::endl;
        std::_Xruntime_error("");
    }

    // Otherwise, store all handles in array
    std::vector<VkPhysicalDevice> devices(numDevices);
    vkEnumeratePhysicalDevices(instance, &numDevices, devices.data());

    // Evaluate each, check if they are suitable
    for (const auto& device : devices) {
        if (isSuitable(device)) {
            GPU = device;
            msaaSamples = getMaxUsableSampleCount(GPU);
            break;
        }
        else {
            std::cout << "not suitable!" << std::endl;
        }
    }

    if (GPU == VK_NULL_HANDLE) {
        std::cout << "could not find a suitable GPU!" << std::endl;
        std::_Xruntime_error("");
    }
}

// Setting up the logical device using the physical device
void VulkanRenderer::createLogicalDevice() {
    QueueFamilyIndices indices = findQueueFamilies(GPU);

    // Create presentation queue with structs
    std::vector<VkDeviceQueueCreateInfo> queuecInfos;
    std::set<uint32_t> uniqueQFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    float queuePrio = 1.0f;
    for (uint32_t queueFamily : uniqueQFamilies) {
        VkDeviceQueueCreateInfo queuecInfo{};
        queuecInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queuecInfo.queueFamilyIndex = queueFamily;
        queuecInfo.queueCount = 1;
        queuecInfo.pQueuePriorities = &queuePrio;
        queuecInfos.push_back(queuecInfo);
    }


    // Specifying device features through another struct
    VkPhysicalDeviceFeatures gpuFeatures{};
    gpuFeatures.samplerAnisotropy = VK_TRUE;

    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelFeature{};
    accelFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    accelFeature.accelerationStructure = VK_TRUE;

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeature{};
    rtPipelineFeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    rtPipelineFeature.rayTracingPipeline = VK_TRUE;
    rtPipelineFeature.pNext = &accelFeature;

    VkPhysicalDeviceBufferDeviceAddressFeaturesKHR BDAfeature{};
    BDAfeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES;
    BDAfeature.bufferDeviceAddress = VK_TRUE;
    BDAfeature.pNext = &rtPipelineFeature;

    VkPhysicalDeviceHostQueryResetFeaturesEXT resetHQfeature{};
    resetHQfeature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES_EXT;
    resetHQfeature.hostQueryReset = VK_TRUE;
    resetHQfeature.pNext = &BDAfeature;

    // Create the logical device, filling in with the create info structs
    VkDeviceCreateInfo deviceCInfo{};
    deviceCInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCInfo.pNext = &resetHQfeature;
    deviceCInfo.queueCreateInfoCount = static_cast<uint32_t>(queuecInfos.size());
    deviceCInfo.pQueueCreateInfos = queuecInfos.data();

    deviceCInfo.pEnabledFeatures = &gpuFeatures;

    // Set enabledLayerCount and ppEnabledLayerNames fields to be compatible with older implementations of Vulkan
    deviceCInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExts.size());
    deviceCInfo.ppEnabledExtensionNames = deviceExts.data();

    // If validation layers are enabled, then fill create info struct with size and name information
    if (enableValLayers) {
        deviceCInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        deviceCInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else {
        deviceCInfo.enabledLayerCount = 0;
    }

    // Instantiate
    if (vkCreateDevice(GPU, &deviceCInfo, nullptr, &device) != VK_SUCCESS) {
        std::_Xruntime_error("failed to instantiate logical device!");
    }

    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
CREATING AND HANDLING IMAGE VIEWS
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderer::createImageViews() {
    SWChainImageViews.resize(SWChainImages.size());

    for (uint32_t i = 0; i < SWChainImages.size(); i++) {
        SWChainImageViews[i] = createImageView(SWChainImages[i], SWChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
CREATING THE RENDER PASS
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderer::createRenderPass() {
    VkAttachmentDescription colorAttachmentDescription{};
    colorAttachmentDescription.format = SWChainImageFormat;
    colorAttachmentDescription.samples = this->msaaSamples;

    // Determine what to do with the data in the attachment before and after the rendering
    colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

    // Apply to stencil data
    colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    // Specify the layout of pixels in memory for the images
    colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachmentDescription{};
    depthAttachmentDescription.format = findDepthFormat();
    depthAttachmentDescription.samples = this->msaaSamples;
    depthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachmentResolve{};
    colorAttachmentResolve.format = this->SWChainImageFormat;
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // One render pass consists of multiple render subpasses, but we are only going to use 1 for the triangle
    VkAttachmentReference colorAttachmentReference{};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentReference{};
    depthAttachmentReference.attachment = 1;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentResolveRef{};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Create the subpass
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    // Have to be explicit about this subpass being a graphics subpass
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentReference;
    subpass.pDepthStencilAttachment = &depthAttachmentReference;
    subpass.pResolveAttachments = &colorAttachmentResolveRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    // Create information struct for the render pass
    std::array<VkAttachmentDescription, 3> attachments = { colorAttachmentDescription, depthAttachmentDescription, colorAttachmentResolve };
    VkRenderPassCreateInfo renderPassCInfo{};
    renderPassCInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassCInfo.pAttachments = attachments.data();
    renderPassCInfo.subpassCount = 1;
    renderPassCInfo.pSubpasses = &subpass;
    renderPassCInfo.dependencyCount = 1;
    renderPassCInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassCInfo, nullptr, &renderPass) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create render pass!");
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
DESCRIPTOR SET LAYOUT
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderer::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding UBOLayoutBinding{};
    UBOLayoutBinding.binding = 0;
    UBOLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    UBOLayoutBinding.descriptorCount = 1;
    UBOLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    UBOLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerLayoutBindingColor{};
    samplerLayoutBindingColor.binding = 0;
    samplerLayoutBindingColor.descriptorCount = 1;
    samplerLayoutBindingColor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBindingColor.pImmutableSamplers = nullptr;
    samplerLayoutBindingColor.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBindingNormal{};
    samplerLayoutBindingNormal.binding = 1;
    samplerLayoutBindingNormal.descriptorCount = 1;
    samplerLayoutBindingNormal.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBindingNormal.pImmutableSamplers = nullptr;
    samplerLayoutBindingNormal.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBindingMetallicRoughness{};
    samplerLayoutBindingMetallicRoughness.binding = 2;
    samplerLayoutBindingMetallicRoughness.descriptorCount = 1;
    samplerLayoutBindingMetallicRoughness.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBindingMetallicRoughness.pImmutableSamplers = nullptr;
    samplerLayoutBindingMetallicRoughness.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBindingAO{};
    samplerLayoutBindingAO.binding = 3;
    samplerLayoutBindingAO.descriptorCount = 1;
    samplerLayoutBindingAO.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBindingAO.pImmutableSamplers = nullptr;
    samplerLayoutBindingAO.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBindingEmission{};
    samplerLayoutBindingEmission.binding = 4;
    samplerLayoutBindingEmission.descriptorCount = 1;
    samplerLayoutBindingEmission.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBindingEmission.pImmutableSamplers = nullptr;
    samplerLayoutBindingEmission.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::vector<VkDescriptorSetLayoutBinding> samplerBindings = { samplerLayoutBindingColor, samplerLayoutBindingNormal, samplerLayoutBindingMetallicRoughness, samplerLayoutBindingAO, samplerLayoutBindingEmission };

    VkDescriptorSetLayoutCreateInfo layoutCInfo{};
    layoutCInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCInfo.bindingCount = 1;
    layoutCInfo.pBindings = &UBOLayoutBinding;

    if (vkCreateDescriptorSetLayout(device, &layoutCInfo, nullptr, &uniformDescriptorSetLayout) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create the uniform descriptor set layout!");
    }

    layoutCInfo.bindingCount = 5;
    layoutCInfo.pBindings = samplerBindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutCInfo, nullptr, &textureDescriptorSetLayout) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create the texture descriptor set layout!");
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
GRAPHICS PIPELINE
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderer::createGraphicsPipeline(ModelHelper* m) {
    // Read the file for the bytecodfe of the shaders
    std::vector<char> vertexShader = readFile("C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/vert.spv");
    std::vector<char> fragmentShader = readFile("C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/frag.spv");

    std::cout << "read files" << std::endl;

    // Wrap the bytecode with VkShaderModule objects
    VkShaderModule vertexShaderModule = createShaderModule(vertexShader);
    VkShaderModule fragmentShaderModule = createShaderModule(fragmentShader);

    //Create the shader information struct to begin actuall using the shader
    VkPipelineShaderStageCreateInfo vertextStageCInfo{};
    vertextStageCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertextStageCInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertextStageCInfo.module = vertexShaderModule;
    vertextStageCInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragmentStageCInfo{};
    fragmentStageCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentStageCInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragmentStageCInfo.module = fragmentShaderModule;
    fragmentStageCInfo.pName = "main";

    // Define array to contain the shader create information structs
    VkPipelineShaderStageCreateInfo stages[] = { vertextStageCInfo, fragmentStageCInfo };

    // Describing the format of the vertex data to be passed to the vertex shader
    VkPipelineVertexInputStateCreateInfo vertexInputCInfo{};
    vertexInputCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputCInfo.vertexBindingDescriptionCount = 1;
    vertexInputCInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputCInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());;
    vertexInputCInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    // Next struct describes what kind of geometry will be drawn from the verts and if primitive restart should be enabled
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCInfo{};
    inputAssemblyCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssemblyCInfo.primitiveRestartEnable = VK_FALSE;

    // Initialize the viewport information struct, a lot of the size information will come from the swap chain extent factor
    VkPipelineViewportStateCreateInfo viewportStateCInfo{};
    viewportStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCInfo.viewportCount = 1;
    viewportStateCInfo.scissorCount = 1;

    // Initialize rasterizer, which takes information from the geometry formed by the vertex shader into fragments to be colored by the fragment shader
    VkPipelineRasterizationStateCreateInfo rasterizerCInfo{};
    rasterizerCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    // Fragments beyond the near and far planes are clamped to those planes, instead of discarding them
    rasterizerCInfo.depthClampEnable = VK_FALSE;
    // If set to true, geometry never passes through the rasterization phase, and disables output to framebuffer
    rasterizerCInfo.rasterizerDiscardEnable = VK_FALSE;
    // Determines how fragments are generated for geometry, using other modes requires enabling a GPU feature
    rasterizerCInfo.polygonMode = VK_POLYGON_MODE_FILL;
    // Linewidth describes thickness of lines in terms of number of fragments 
    rasterizerCInfo.lineWidth = 1.0f;
    // Specify type of culling and and the vertex order for the faces to be considered
    rasterizerCInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizerCInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    // Alter depth values by adding constant or biasing them based on a fragment's slope
    rasterizerCInfo.depthBiasEnable = VK_FALSE;

    // Multisampling information struct
    VkPipelineMultisampleStateCreateInfo multiSamplingCInfo{};
    multiSamplingCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multiSamplingCInfo.sampleShadingEnable = VK_FALSE;
    multiSamplingCInfo.rasterizationSamples = this->msaaSamples;

    // Depth and stencil testing would go here, but not doing this for the triangle
    VkPipelineDepthStencilStateCreateInfo depthStencilCInfo{};
    depthStencilCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilCInfo.depthTestEnable = VK_TRUE;
    depthStencilCInfo.depthWriteEnable = VK_TRUE;
    depthStencilCInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilCInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilCInfo.stencilTestEnable = VK_FALSE;

    // Color blending - color from fragment shader needs to be combined with color already in the framebuffer
    // If <blendEnable> is set to false, then the color from the fragment shader is passed through to the framebuffer
    // Otherwise, combine with a colorWriteMask to determine the channels that are passed through
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    // Array of structures for all of the framebuffers to set blend constants as blend factors
    VkPipelineColorBlendStateCreateInfo colorBlendingCInfo{};
    colorBlendingCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendingCInfo.logicOpEnable = VK_FALSE;
    colorBlendingCInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendingCInfo.attachmentCount = 1;
    colorBlendingCInfo.pAttachments = &colorBlendAttachment;
    colorBlendingCInfo.blendConstants[0] = 0.0f;
    colorBlendingCInfo.blendConstants[1] = 0.0f;
    colorBlendingCInfo.blendConstants[2] = 0.0f;
    colorBlendingCInfo.blendConstants[3] = 0.0f;

    // Not much can be changed without completely recreating the rendering pipeline, so we fill in a struct with the information
    std::vector<VkDynamicState> dynaStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCInfo{};
    dynamicStateCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCInfo.dynamicStateCount = static_cast<uint32_t>(dynaStates.size());
    dynamicStateCInfo.pDynamicStates = dynaStates.data();

    VkDescriptorSetLayout descSetLayouts[] = { uniformDescriptorSetLayout, textureDescriptorSetLayout };

    VkPushConstantRange pcRange{};
    pcRange.offset = 0;
    pcRange.size = sizeof(glm::mat4);
    pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    // We can use uniform values to make changes to the shaders without having to create them again, similar to global variables
    // Initialize the pipeline layout with another create info struct
    VkPipelineLayoutCreateInfo pipeLineLayoutCInfo{};
    pipeLineLayoutCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeLineLayoutCInfo.setLayoutCount = 2;
    pipeLineLayoutCInfo.pSetLayouts = descSetLayouts;
    pipeLineLayoutCInfo.pushConstantRangeCount = 1;
    pipeLineLayoutCInfo.pPushConstantRanges = &pcRange;

    if (vkCreatePipelineLayout(device, &pipeLineLayoutCInfo, nullptr, &pipeLineLayout) != VK_SUCCESS) {
        std::cout << "nah you buggin" << std::endl;
        std::_Xruntime_error("Failed to create pipeline layout!");
    }

    std::cout << "pipeline layout created" << std::endl;

    // Combine the shader stages, fixed-function state, pipeline layout, and render pass to create the graphics pipeline
    // First - populate struct with the information
    VkGraphicsPipelineCreateInfo graphicsPipelineCInfo{};
    graphicsPipelineCInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    graphicsPipelineCInfo.stageCount = 2;
    graphicsPipelineCInfo.pStages = stages;

    graphicsPipelineCInfo.pVertexInputState = &vertexInputCInfo;
    graphicsPipelineCInfo.pInputAssemblyState = &inputAssemblyCInfo;
    graphicsPipelineCInfo.pViewportState = &viewportStateCInfo;
    graphicsPipelineCInfo.pRasterizationState = &rasterizerCInfo;
    graphicsPipelineCInfo.pMultisampleState = &multiSamplingCInfo;
    graphicsPipelineCInfo.pDepthStencilState = &depthStencilCInfo;
    graphicsPipelineCInfo.pColorBlendState = &colorBlendingCInfo;
    graphicsPipelineCInfo.pDynamicState = &dynamicStateCInfo;

    graphicsPipelineCInfo.layout = pipeLineLayout;

    graphicsPipelineCInfo.renderPass = renderPass;
    graphicsPipelineCInfo.subpass = 0;

    graphicsPipelineCInfo.basePipelineHandle = VK_NULL_HANDLE;

    // POI: Instead if using a few fixed pipelines, we create one pipeline for each material using the properties of that material
    for (auto& material : m->mats) {

        struct MaterialSpecializationData {
            VkBool32 alphaMask;
            float alphaMaskCutoff;
        } materialSpecializationData;

        materialSpecializationData.alphaMask = material.alphaMode == "MASK";
        materialSpecializationData.alphaMaskCutoff = material.alphaCutOff;

        // POI: Constant fragment shader material parameters will be set using specialization constants
        VkSpecializationMapEntry me{};
        me.constantID = 0;
        me.offset = offsetof(MaterialSpecializationData, alphaMask);
        me.size = sizeof(MaterialSpecializationData::alphaMask);

        VkSpecializationMapEntry metoo{};
        metoo.constantID = 1;
        metoo.offset = offsetof(MaterialSpecializationData, alphaMaskCutoff);
        metoo.size = sizeof(MaterialSpecializationData::alphaMaskCutoff);

        std::vector<VkSpecializationMapEntry> specializationMapEntries = { me, metoo };

        VkSpecializationInfo specializationInfo{};
        specializationInfo.dataSize = sizeof(materialSpecializationData);
        specializationInfo.mapEntryCount = static_cast<uint32_t>(specializationMapEntries.size());
        specializationInfo.pData = &materialSpecializationData;
        specializationInfo.pMapEntries = specializationMapEntries.data();

        stages[1].pSpecializationInfo = &specializationInfo;

        // For double sided materials, culling will be disabled
        rasterizerCInfo.cullMode = material.doubleSides ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;

        // Create the object
        VkResult res3 = vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &graphicsPipelineCInfo, nullptr, &(material.pipeline));
        if (res3 != VK_SUCCESS) {
            std::cout << "failed to create graphicspipeline" << std::endl;
            std::_Xruntime_error("Failed to create the graphics pipeline!");
        }
    }

    std::cout << "pipeline created" << std::endl;

    // After all the processing with the modules is over, destroy them
    vkDestroyShaderModule(device, vertexShaderModule, nullptr);
    vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
    
    std::cout << "destroyed modules" << std::endl;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
HELPER METHODS FOR THE GRAPHICS PIPELINE
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector<char> VulkanRenderer::readFile(const std::string& filename) {
    // Start reading at end of the file and read as binary
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        std::cout << "failed to open file" << std::endl;
        std::_Xruntime_error("");
    }

    // Read the file, create the buffer, and return it
    size_t fileSize = file.tellg();
    std::vector<char> buffer((size_t)file.tellg());
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

// In order to pass the binary code to the graphics pipeline, we need to create a VkShaderModule object to wrap it with
VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char>& binary) {
    // We need to specify a pointer to the buffer with the bytecode and the length of the bytecode. Bytecode pointer is a uint32_t pointer
    VkShaderModuleCreateInfo shaderModuleCInfo{};
    shaderModuleCInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCInfo.codeSize = binary.size();
    shaderModuleCInfo.pCode = reinterpret_cast<const uint32_t*>(binary.data());

    VkShaderModule shaderMod;
    if (vkCreateShaderModule(device, &shaderModuleCInfo, nullptr, &shaderMod)) {
        std::_Xruntime_error("Failed to create a shader module!");
    }

    return shaderMod;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
PRIVATE HELPERS
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

uint32_t VulkanRenderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(GPU, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    std::_Xruntime_error("Failed to find a suitable memory type!");
}

VkImageView VulkanRenderer::createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels){
    VkImageViewCreateInfo imageViewCInfo{};
    imageViewCInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCInfo.image = image;
    imageViewCInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCInfo.format = format;
    imageViewCInfo.subresourceRange.aspectMask = aspectFlags;
    imageViewCInfo.subresourceRange.baseMipLevel = 0;
    imageViewCInfo.subresourceRange.levelCount = mipLevels;
    imageViewCInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCInfo.subresourceRange.layerCount = 1;

    VkImageView tempImageView;
    if (vkCreateImageView(device, &imageViewCInfo, nullptr, &tempImageView) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create a texture image view!");
    }

    return tempImageView;
}

void VulkanRenderer::createImage(uint32_t width, uint32_t height, uint32_t mipLevel, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
    VkImageCreateInfo imageCInfo{};
    imageCInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCInfo.extent.width = width;
    imageCInfo.extent.height = height;
    imageCInfo.extent.depth = 1;
    imageCInfo.mipLevels = mipLevel;
    imageCInfo.arrayLayers = 1;

    imageCInfo.format = format;
    imageCInfo.tiling = tiling;
    imageCInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCInfo.usage = usage;
    imageCInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCInfo.samples = numSamples;

    if (vkCreateImage(device, &imageCInfo, nullptr, &image) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create an image!");
    }

    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(device, image, &memoryRequirements);

    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.allocationSize = memoryRequirements.size;
    allocateInfo.memoryTypeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocateInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        std::cout << "couldn't allocate lol" << std::endl;
        std::_Xruntime_error("Failed to allocate the image memory!");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
CREATING THE FRAME BUFFER
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderer::createFrameBuffer() {
    SWChainFrameBuffers.resize(SWChainImageViews.size());

    // Iterate through the image views and create framebuffers from them
    for (size_t i = 0; i < SWChainImageViews.size(); i++) {
        std::array<VkImageView, 3> attachments = { colorImageView, depthImageView, SWChainImageViews[i]};

        VkFramebufferCreateInfo frameBufferCInfo{};
        frameBufferCInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frameBufferCInfo.renderPass = renderPass;
        frameBufferCInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        frameBufferCInfo.pAttachments = attachments.data();
        frameBufferCInfo.width = SWChainExtent.width;
        frameBufferCInfo.height = SWChainExtent.height;
        frameBufferCInfo.layers = 1;

        if (vkCreateFramebuffer(device, &frameBufferCInfo, nullptr, &SWChainFrameBuffers[i]) != VK_SUCCESS) {
            std::_Xruntime_error("Failed to create a framebuffer for an image view!");
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
CREATE THE VERTEX, INDEX, AND UNIFORM BUFFERS AND OTHER HELPER METHODS
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

VkCommandBuffer VulkanRenderer::beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandPool = commandPool;
    allocateInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocateInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void VulkanRenderer::endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    std::cout << "submitted command buffer" << std::endl;

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);

    std::cout << ("freed command buffer: ");
}

void VulkanRenderer::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferCInfo{};
    bufferCInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCInfo.size = size;
    bufferCInfo.usage = usage;
    bufferCInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferCInfo, nullptr, &buffer) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create the vertex buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo{};
    memoryAllocateFlagsInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO;
    memoryAllocateFlagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT_KHR;

    VkMemoryAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.pNext = &memoryAllocateFlagsInfo;
    allocateInfo.allocationSize = memRequirements.size;
    allocateInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(device, &allocateInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void VulkanRenderer::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}

void VulkanRenderer::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(SWChainImages.size());
    uniformBuffersMemory.resize(SWChainImages.size());

    for (size_t i = 0; i < SWChainImages.size(); i++) {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
    }
}

void VulkanRenderer::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(SWChainImages.size());
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    // Multiplied by 2 for imgui, needs to render separate font atlas, so needs double the image space
    poolSizes[1].descriptorCount = this->numMats * 5 * static_cast<uint32_t>(SWChainImages.size());

    VkDescriptorPoolCreateInfo poolCInfo{};
    poolCInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolCInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolCInfo.pPoolSizes = poolSizes.data();
    //  needs to render separate font atlas ( + 1)
    poolCInfo.maxSets = this->numImages * static_cast<uint32_t>(SWChainImages.size()) + 1;

    if (vkCreateDescriptorPool(device, &poolCInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create the descriptor pool!");
    }
}

void VulkanRenderer::createDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(SWChainImages.size(), uniformDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = descriptorPool;
    allocateInfo.descriptorSetCount = static_cast<uint32_t>(SWChainImages.size());
    allocateInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(SWChainImages.size());
    if (vkAllocateDescriptorSets(device, &allocateInfo, descriptorSets.data()) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < SWChainImages.size(); i++) {
        VkDescriptorBufferInfo descriptorBufferInfo{};
        descriptorBufferInfo.buffer = uniformBuffers[i];
        descriptorBufferInfo.offset = 0;
        descriptorBufferInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet descriptorWriteSet{};
        descriptorWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWriteSet.dstSet = descriptorSets[i];
        descriptorWriteSet.dstBinding = 0;
        descriptorWriteSet.dstArrayElement = 0;
        descriptorWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWriteSet.descriptorCount = 1;
        descriptorWriteSet.pBufferInfo = &descriptorBufferInfo;

        vkUpdateDescriptorSets(device, 1, &descriptorWriteSet, 0, nullptr);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
CREATE THE DEPTH RESOURCES
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkFormat VulkanRenderer::findDepthFormat() {
    return findSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

VkFormat VulkanRenderer::findSupportedFormat(const std::vector<VkFormat>& potentialFormats, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : potentialFormats) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(GPU, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    std::_Xruntime_error("Failed to find a supported format!");
}

void VulkanRenderer::createColorResources() {
    VkFormat colorFormat = SWChainImageFormat;

    createImage(SWChainExtent.width, SWChainExtent.height, 1, this->msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory);
    colorImageView = createImageView(colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

void VulkanRenderer::createDepthResources() {
    VkFormat depthFormat = findDepthFormat();
    createImage(SWChainExtent.width, SWChainExtent.height, 1, this->msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
    depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
CREATING THE COMMAND POOL AND BUFFER
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderer::createCommandPool() {
    QFIndices = findQueueFamilies(GPU);

    // Creating the command pool create information struct
    VkCommandPoolCreateInfo commandPoolCInfo{};
    commandPoolCInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCInfo.queueFamilyIndex = QFIndices.graphicsFamily.value();

    // Actual creation of the command buffer
    if (vkCreateCommandPool(device, &commandPoolCInfo, nullptr, &commandPool) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create a command pool!");
    }
}

void VulkanRenderer::createCommandBuffers(int numFramesInFlight) {
    commandBuffers.resize(numFramesInFlight);

    // Information to allocate the frame buffer
    VkCommandBufferAllocateInfo CBAllocateInfo{};
    CBAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    CBAllocateInfo.commandPool = commandPool;
    CBAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CBAllocateInfo.commandBufferCount = (uint32_t)commandBuffers.size();

    if (vkAllocateCommandBuffers(device, &CBAllocateInfo, commandBuffers.data()) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to allocate a command buffer!");
    }
}

void VulkanRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    // Start command buffer recording
    VkCommandBufferBeginInfo CBBeginInfo{};
    CBBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CBBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffer, &CBBeginInfo) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to start recording with the command buffer!");
    }

    // Start the render pass
    VkRenderPassBeginInfo RPBeginInfo{};
    // Create the render pass
    RPBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    RPBeginInfo.renderPass = renderPass;
    RPBeginInfo.framebuffer = SWChainFrameBuffers[imageIndex];
    // Define the size of the render area
    RPBeginInfo.renderArea.offset = { 0, 0 };
    RPBeginInfo.renderArea.extent = SWChainExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = clearValue.color;
    clearValues[1].depthStencil = { 1.0f, 0 };

    // Define the clear values to use
    RPBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());;
    RPBeginInfo.pClearValues = clearValues.data();

    // Finally, begin the render pass
    vkCmdBeginRenderPass(commandBuffer, &RPBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)SWChainExtent.width;
    viewport.height = (float)SWChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = SWChainExtent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeLineLayout, 0, 1, &descriptorSets[this->currentFrame], 0, nullptr);

    for (int i = 0; i < this->numModels; i++) {
        models[i]->render(commandBuffer, pipeLineLayout);
    }
}

void VulkanRenderer::postDrawEndCommandBuffer(VkCommandBuffer commandBuffer, SDL_Window* window, int maxFramesInFlight) {

    // After drawing is over, end the render pass
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        std::cout << "recording failed, you bum!" << std::endl;
        std::_Xruntime_error("Failed to record back the command buffer!");
    }


    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    // Submit the command buffer with the semaphore
    VkSubmitInfo queueSubmitInfo{};
    queueSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { this->imageAcquiredSema[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    queueSubmitInfo.waitSemaphoreCount = 1;
    queueSubmitInfo.pWaitSemaphores = waitSemaphores;
    queueSubmitInfo.pWaitDstStageMask = waitStages;

    // Specify the command buffers to actually submit for execution
    queueSubmitInfo.commandBufferCount = 1;
    queueSubmitInfo.pCommandBuffers = &this->commandBuffers[currentFrame];

    // Specify which semaphores to signal once command buffers have finished execution
    VkSemaphore signaledSemaphores[] = { this->renderedSema[currentFrame] };
    queueSubmitInfo.signalSemaphoreCount = 1;
    queueSubmitInfo.pSignalSemaphores = signaledSemaphores;

    // Finally, submit the queue info
    if (vkQueueSubmit(this->graphicsQueue, 1, &queueSubmitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        std::cout << "failed submit the draw command buffer, ???" << std::endl;
        std::_Xruntime_error("Failed to submit the draw command buffer to the graphics queue!");
    }

    // Present the frame from the queue
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    // Specify the semaphores to wait on before presentation happens
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signaledSemaphores;

    // Specify the swap chains to present images and the image index for each chain
    VkSwapchainKHR swapChains[] = { this->swapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    VkResult res2 = vkQueuePresentKHR(this->presentQueue, &presentInfo);

    if (res2 == VK_ERROR_OUT_OF_DATE_KHR || res2 == VK_SUBOPTIMAL_KHR || this->frBuffResized) {
        this->frBuffResized = false;
        this->recreateSwapChain(window);
    }
    else if (res2 != VK_SUCCESS) {
        std::_Xruntime_error("Failed to present a swap chain image!");
    }

    currentFrame = (currentFrame + 1) % maxFramesInFlight;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
INITIALIZING THE TWO SEMAPHORES AND THE FENCES
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderer::createSemaphores(const int maxFramesInFlight) {
    imageAcquiredSema.resize(maxFramesInFlight);
    renderedSema.resize(maxFramesInFlight);
    inFlightFences.resize(maxFramesInFlight);

    VkSemaphoreCreateInfo semaCInfo{};
    semaCInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceCInfo{};
    fenceCInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < maxFramesInFlight; i++) {
        if (vkCreateSemaphore(device, &semaCInfo, nullptr, &imageAcquiredSema[i]) != VK_SUCCESS || vkCreateSemaphore(device, &semaCInfo, nullptr, &renderedSema[i]) != VK_SUCCESS || vkCreateFence(device, &fenceCInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            std::cout << "bum" << std::endl;
            std::_Xruntime_error("Failed to create the synchronization objects for a frame!");
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
SWAPCHAIN RECREATION
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderer::cleanupSWChain() {
    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);

    vkDestroyImageView(device, colorImageView, nullptr);
    vkDestroyImage(device, colorImage, nullptr);
    vkFreeMemory(device, colorImageMemory, nullptr);

    for (size_t i = 0; i < SWChainFrameBuffers.size(); i++) {
        vkDestroyFramebuffer(device, SWChainFrameBuffers[i], nullptr);
    }

    for (size_t i = 0; i < SWChainImageViews.size(); i++) {
        vkDestroyImageView(device, SWChainImageViews[i], nullptr);
    }

    vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void VulkanRenderer::recreateSwapChain(SDL_Window* window) {
    int width = 0, height = 0;
    SDL_GL_GetDrawableSize(window, &width, &height);
    while (width == 0 || height == 0) {
        SDL_GL_GetDrawableSize(window, &width, &height);
    }
    vkDeviceWaitIdle(device);

    cleanupSWChain();

    createSWChain(window);
    createImageViews();
    createColorResources();
    createDepthResources();
    createFrameBuffer();
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
FREEING
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderer::freeEverything(int framesInFlight) {
    cleanupSWChain();

    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);

    vkDestroyImageView(device, colorImageView, nullptr);
    vkDestroyImage(device, colorImage, nullptr);
    vkFreeMemory(device, colorImageMemory, nullptr);

    for (size_t i = 0; i < framesInFlight; i++) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
    }

    vkDestroyDescriptorPool(device, descriptorPool, nullptr);

    for (int i = 0; i < numModels; i++) {
        models[i]->destroy();
        delete models[i];
    }

    for (size_t i = 0; i < framesInFlight; i++) {
        vkDestroySemaphore(device, renderedSema[i], nullptr);
        vkDestroySemaphore(device, imageAcquiredSema[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    vkDestroyPipeline(device, graphicsPipeline, nullptr);
    vkDestroyPipelineLayout(device, pipeLineLayout, nullptr);
    vkDestroyRenderPass(device, renderPass, nullptr);

    for (auto imageView : SWChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroyCommandPool(device, commandPool, nullptr);

    vkDestroyDevice(device, nullptr);

    if (enableValLayers) {
        DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}































































///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
MODEL HELPER PRIVATE HELPER METHODS
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ModelHelper::createVertexBuffer() {
    VkDeviceSize bufferSize = sizeof(this->model.vertices[0]) * this->model.vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    this->vkR->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(this->vkR->device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, this->model.vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(this->vkR->device, stagingBufferMemory);

    this->vkR->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, this->vertexBuffer, this->vertexBufferMemory);
    this->vkR->copyBuffer(stagingBuffer, this->vertexBuffer, bufferSize);
    std::cout << "model vertex buffer" << std::endl;

    vkDestroyBuffer(this->vkR->device, stagingBuffer, nullptr);
    vkFreeMemory(this->vkR->device, stagingBufferMemory, nullptr);
}

void ModelHelper::createIndexBuffer() {
    VkDeviceSize bufferSize = sizeof(this->model.indices[0]) * this->model.indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    this->vkR->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(this->vkR->device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, this->model.indices.data(), (size_t)bufferSize);
    vkUnmapMemory(this->vkR->device, stagingBufferMemory);

    this->vkR->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
    this->vkR->copyBuffer(stagingBuffer, indexBuffer, bufferSize);
    std::cout << "model index buffer" << std::endl;

    vkDestroyBuffer(this->vkR->device, stagingBuffer, nullptr);
    vkFreeMemory(this->vkR->device, stagingBufferMemory, nullptr);
}

// LOAD FUNCTIONS TEMPLATED FROM GLTFLOADING EXAMPLE ON GITHUB BY SASCHA WILLEMS
void loadImages(tinygltf::Model& in, std::vector<TextureHelper*>& images, VulkanRenderer* vkR) {
    for (size_t i = 0; i < in.images.size(); i++) {
        TextureHelper* tex = new TextureHelper(in, (int)i);
        tex->vkR = vkR;
        tex->load();
        images.push_back(tex);
    }
    TextureHelper* dummyAO = new TextureHelper(in, -1);
    dummyAO->vkR = vkR;
    TextureHelper* dummyMetallic = new TextureHelper(in, -2);
    dummyMetallic->vkR = vkR;
    TextureHelper* dummyNormal = new TextureHelper(in, -3);
    dummyNormal->vkR = vkR;
    dummyAO->load();
    dummyMetallic->load();
    dummyNormal->load();
    images.push_back(dummyNormal);
    images.push_back(dummyMetallic);
    images.push_back(dummyAO);
}

void loadMaterials(tinygltf::Model& in, std::vector<Material>& mats, ModelHelper* mod) {
    uint32_t numImages = in.images.size();
    std::cout << numImages << std::endl;
    uint32_t dummyNormalIndex = numImages;
    uint32_t dummyMetallicRoughnessIndex = numImages + 1;
    uint32_t dummyAOIndex = numImages + 2;
    mats.resize(in.materials.size());
    int count = 0;
    std::cout << mats.size() << std::endl;
    for (Material& m : mats) {
        tinygltf::Material gltfMat = in.materials[count];
        if (gltfMat.values.find("baseColorFactor") != gltfMat.values.end()) {
            m.baseColor = glm::make_vec4(gltfMat.values["baseColorFactor"].ColorFactor().data());
        }
        if (gltfMat.values.find("baseColorTexture") != gltfMat.values.end()) {
            m.baseColorTexIndex = gltfMat.values["baseColorTexture"].TextureIndex();
            mod->images[mod->textures[m.baseColorTexIndex].textureIndex]->imageFormat = VK_FORMAT_R8G8B8A8_SRGB;

        }
        if (gltfMat.additionalValues.find("normalTexture") != gltfMat.additionalValues.end()) {
            m.normalTexIndex = gltfMat.additionalValues["normalTexture"].TextureIndex();
        }
        else {
            m.normalTexIndex = dummyNormalIndex;
        }
        if (gltfMat.values.find("metallicRoughnessTexture") != gltfMat.values.end()) {
            m.metallicRoughnessIndex = gltfMat.values["metallicRoughnessTexture"].TextureIndex();
        }
        else {
            m.metallicRoughnessIndex = dummyMetallicRoughnessIndex;
        }
        if (gltfMat.additionalValues.find("occlusionTexture") != gltfMat.additionalValues.end()) {
            m.aoIndex = gltfMat.additionalValues["occlusionTexture"].TextureIndex();
        }
        else {
            m.aoIndex = dummyAOIndex;
        }
        if (gltfMat.additionalValues.find("emissiveTexture") != gltfMat.additionalValues.end()) {
            m.emissionIndex = gltfMat.additionalValues["emissiveTexture"].TextureIndex();
            mod->images[mod->textures[m.emissionIndex].textureIndex]->imageFormat = VK_FORMAT_R8G8B8A8_SRGB;

        }
        else {
            m.emissionIndex = dummyMetallicRoughnessIndex;
        }
        mod->images[mod->textures[m.normalTexIndex].textureIndex]->imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
        mod->images[mod->textures[m.metallicRoughnessIndex].textureIndex]->imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
        mod->images[mod->textures[m.aoIndex].textureIndex]->imageFormat = VK_FORMAT_R8G8B8A8_UNORM;

        m.alphaMode = gltfMat.alphaMode;
        m.alphaCutOff = (float)gltfMat.alphaCutoff;
        m.doubleSides = gltfMat.doubleSided;
        count++;
    }
}

void loadTextures(tinygltf::Model& in, std::vector<TextureIndexHolder>& textures) {
    textures.resize(in.textures.size());
    uint32_t size = textures.size();
    for (size_t i = 0; i < in.textures.size(); i++) {
        textures[i].textureIndex = in.textures[i].source;
    }
    textures.push_back(TextureIndexHolder{ size });
    textures.push_back(TextureIndexHolder{ size + 1 });
    textures.push_back(TextureIndexHolder{ size + 2 });
}

void loadNode(tinygltf::Model& in, const tinygltf::Node& nodeIn, Scene::SceneNode* parent, Scene& model, std::vector<Scene::SceneNode*>& nodes) {
    Scene::SceneNode* scNode = new Scene::SceneNode{};
    scNode->transform = glm::mat4(1.0f);
    scNode->parent = parent;

    if (nodeIn.translation.size() == 3) {
        scNode->transform = glm::translate(scNode->transform, glm::vec3(glm::make_vec3(nodeIn.translation.data())));
    }
    if (nodeIn.rotation.size() == 4) {
        glm::quat q = glm::make_quat(nodeIn.rotation.data());
        scNode->transform *= glm::mat4(q);
    }
    if (nodeIn.scale.size() == 3) {
        scNode->transform = glm::scale(scNode->transform, glm::vec3(glm::make_vec3(nodeIn.scale.data())));
    }
    if (nodeIn.matrix.size() == 16) {
        scNode->transform = glm::make_mat4x4(nodeIn.matrix.data());
    }

    if (nodeIn.children.size() > 0) {
        for (size_t i = 0; i < nodeIn.children.size(); i++) {
            loadNode(in, in.nodes[nodeIn.children[i]], scNode, model, nodes);
        }
    }

    if (nodeIn.mesh > -1) {
        const tinygltf::Mesh mesh = in.meshes[nodeIn.mesh];
        for (size_t i = 0; i < mesh.primitives.size(); i++) {
            const tinygltf::Primitive& gltfPrims = mesh.primitives[i];
            uint32_t firstIndex = static_cast<uint32_t>(model.indices.size());
            uint32_t vertexStart = static_cast<uint32_t>(model.vertices.size());
            uint32_t indexCount = 0;
            uint32_t numVertices = 0;

            // FOR VERTICES
            const float* positionBuff = nullptr;
            const float* normalsBuff = nullptr;
            const float* uvBuff = nullptr;
            const float* tangentsBuff = nullptr;
            if (gltfPrims.attributes.find("POSITION") != gltfPrims.attributes.end()) {
                const tinygltf::Accessor& accessor = in.accessors[gltfPrims.attributes.find("POSITION")->second];
                const tinygltf::BufferView& view = in.bufferViews[accessor.bufferView];
                positionBuff = reinterpret_cast<const float*>(&(in.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                numVertices = accessor.count;
            }
            if (gltfPrims.attributes.find("NORMAL") != gltfPrims.attributes.end()) {
                const tinygltf::Accessor& accessor = in.accessors[gltfPrims.attributes.find("NORMAL")->second];
                const tinygltf::BufferView& view = in.bufferViews[accessor.bufferView];
                normalsBuff = reinterpret_cast<const float*>(&(in.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
            }
            if (gltfPrims.attributes.find("TEXCOORD_0") != gltfPrims.attributes.end()) {
                const tinygltf::Accessor& accessor = in.accessors[gltfPrims.attributes.find("TEXCOORD_0")->second];
                const tinygltf::BufferView& view = in.bufferViews[accessor.bufferView];
                uvBuff = reinterpret_cast<const float*>(&(in.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
            }
            if (gltfPrims.attributes.find("TANGENT") != gltfPrims.attributes.end()) {
                const tinygltf::Accessor& accessor = in.accessors[gltfPrims.attributes.find("TANGENT")->second];
                const tinygltf::BufferView& view = in.bufferViews[accessor.bufferView];
                tangentsBuff = reinterpret_cast<const float*>(&(in.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
            }

            for (size_t vert = 0; vert < numVertices; vert++) {
                Vertex v;
                v.pos = glm::vec4(glm::make_vec3(&positionBuff[vert * 3]), 1.0f);
                v.normal = glm::normalize(glm::vec3(normalsBuff ? glm::make_vec3(&normalsBuff[vert * 3]) : glm::vec3(0.0f)));
                v.uv = uvBuff ? glm::make_vec2(&uvBuff[vert * 2]) : glm::vec3(0.0f);
                v.color = glm::vec3(1.0f);
                v.tangent = tangentsBuff ? glm::make_vec4(&tangentsBuff[vert * 4]) : glm::vec4(0.0f);
                model.vertices.push_back(v);
                model.totalVertices++;
            }

            // FOR INDEX
            const tinygltf::Accessor& accessor = in.accessors[gltfPrims.indices];
            const tinygltf::BufferView& view = in.bufferViews[accessor.bufferView];
            const tinygltf::Buffer& buffer = in.buffers[view.buffer];

            indexCount += static_cast<uint32_t>(accessor.count);

            switch (accessor.componentType) {
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                const uint32_t* buf = reinterpret_cast<const uint32_t*>(&buffer.data[accessor.byteOffset + view.byteOffset]);
                for (size_t index = 0; index < accessor.count; index++) {
                    model.indices.push_back(buf[index] + vertexStart);
                }
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                const uint16_t* buf = reinterpret_cast<const uint16_t*>(&buffer.data[accessor.byteOffset + view.byteOffset]);
                for (size_t index = 0; index < accessor.count; index++) {
                    model.indices.push_back(buf[index] + vertexStart);
                }
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                const uint8_t* buf = reinterpret_cast<const uint8_t*>(&buffer.data[accessor.byteOffset + view.byteOffset]);
                for (size_t index = 0; index < accessor.count; index++) {
                    model.indices.push_back(buf[index] + vertexStart);
                }
                break;
            }
            default:
                std::cout << "index component type not supported" << std::endl;
                std::_Xruntime_error("index component type not supported");
            }
            PrimitiveObjIndices p;
            p.firstIndex = firstIndex;
            p.numIndices = indexCount;
            model.totalIndices += indexCount;
            p.materialIndex = gltfPrims.material;
            scNode->mesh.primitives.push_back(p);
        }
    }

    if (parent) {
        parent->children.push_back(scNode);
    }
    else {
        nodes.push_back(scNode);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
MODEL HELPER PUBLIC METHODS
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ModelHelper::ModelHelper(std::string path) {
    this->modPath = path;
}

void ModelHelper::loadGLTF() {
    tinygltf::Model in;
    tinygltf::TinyGLTF gltfContext;
    std::string error, warning;

    bool loadedFile = false;
    std::filesystem::path fPath = this->modPath;
    if (fPath.extension() == ".glb") {
        loadedFile = gltfContext.LoadBinaryFromFile(&(in), &error, &warning, this->modPath);
    }
    else if (fPath.extension() == ".gltf") {
        loadedFile = gltfContext.LoadASCIIFromFile(&(in), &error, &warning, this->modPath);
    }

    if (loadedFile) {
        loadImages(in, this->images, this->vkR);
        this->textures.resize(in.textures.size() + 3);
        loadMaterials(in, this->mats, this);
        loadTextures(in, this->textures);

        const tinygltf::Scene& scene = in.scenes[0];
        for (size_t i = 0; i < scene.nodes.size(); i++) {
            const tinygltf::Node node = in.nodes[scene.nodes[i]];
            loadNode(in, node, nullptr, this->model, this->nodes);
        }
    }
    else {
        std::cout << "couldnt open gltf file" << std::endl;
        std::_Xruntime_error("Failed to create the vertex buffer!");
    }

    createVertexBuffer();

    createIndexBuffer();
}

void ModelHelper::destroy() {
    vkDestroyBuffer(this->vkR->device, vertexBuffer, nullptr);
    vkDestroyBuffer(this->vkR->device, indexBuffer, nullptr);

    vkFreeMemory(this->vkR->device, vertexBufferMemory, nullptr);
    vkFreeMemory(this->vkR->device, indexBufferMemory, nullptr);
}

void ModelHelper::drawIndexed(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, Scene::SceneNode* node) {
    if (node->mesh.primitives.size() > 0) {
        glm::mat4 nodeTransform = node->transform;
        Scene::SceneNode* curParent = node->parent;
        while (curParent) {
            nodeTransform = curParent->transform * nodeTransform;
            curParent = curParent->parent;
        }
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &nodeTransform);
        for (PrimitiveObjIndices p : node->mesh.primitives) {
            if (p.numIndices > 0) {
                Material mat = this->mats[p.materialIndex];
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mat.pipeline);
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &(mat.descriptorSet), 0, nullptr);
                vkCmdDrawIndexed(commandBuffer, p.numIndices, 1, p.firstIndex, 0, 0);
            }
        }
    }
    for (auto& child : node->children) {
        drawIndexed(commandBuffer, pipelineLayout, child);
    }
}

void ModelHelper::render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) {
    VkBuffer vertexBuffers[] = { this->vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    for (auto& node : this->nodes) {
        this->drawIndexed(commandBuffer, pipelineLayout, node);
    }
}

void createTextureDescriptorSet(VulkanRenderer* vkR, TextureHelper* t) {
    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = vkR->descriptorPool;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &vkR->textureDescriptorSetLayout;

    VkResult res = vkAllocateDescriptorSets(vkR->device, &allocateInfo, &(t->descriptorSet));
    if (res != VK_SUCCESS) {
        std::_Xruntime_error("Failed to allocate descriptor sets!");
    }

    VkDescriptorImageInfo imageInfo{};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = t->textureImageView;
    imageInfo.sampler = t->textureSampler;

    VkWriteDescriptorSet descriptorWriteSet{};

    descriptorWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWriteSet.dstSet = t->descriptorSet;
    descriptorWriteSet.dstBinding = 0;
    descriptorWriteSet.dstArrayElement = 0;
    descriptorWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWriteSet.descriptorCount = 1;
    descriptorWriteSet.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(vkR->device, 1, &descriptorWriteSet, 0, nullptr);
}

void ModelHelper::createDescriptors() {
    for (Material& m : this->mats) {
        VkDescriptorSetAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.descriptorPool = this->vkR->descriptorPool;
        allocateInfo.descriptorSetCount = 1;
        allocateInfo.pSetLayouts = &(this->vkR->textureDescriptorSetLayout);

        VkResult res = vkAllocateDescriptorSets(vkR->device, &allocateInfo, &(m.descriptorSet));
        if (res != VK_SUCCESS) {
            std::_Xruntime_error("Failed to allocate descriptor sets!");
        }

        TextureHelper* t = this->images[this->textures[m.baseColorTexIndex].textureIndex];
        VkDescriptorImageInfo colorImageInfo{};
        colorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        colorImageInfo.imageView = t->textureImageView;
        colorImageInfo.sampler = t->textureSampler;

        TextureHelper* n = this->images[this->textures[m.normalTexIndex].textureIndex];
        VkDescriptorImageInfo normalImageInfo{};
        normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        normalImageInfo.imageView = n->textureImageView;
        normalImageInfo.sampler = n->textureSampler;

        TextureHelper* mR = this->images[this->textures[m.metallicRoughnessIndex].textureIndex];
        VkDescriptorImageInfo mrImageInfo{};
        mrImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        mrImageInfo.imageView = mR->textureImageView;
        mrImageInfo.sampler = mR->textureSampler;

        TextureHelper* ao = this->images[this->textures[m.aoIndex].textureIndex];
        VkDescriptorImageInfo aoImageInfo{};
        aoImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        aoImageInfo.imageView = ao->textureImageView;
        aoImageInfo.sampler = ao->textureSampler;

        TextureHelper* em = this->images[this->textures[m.emissionIndex].textureIndex];
        VkDescriptorImageInfo emissionImageInfo{};
        emissionImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        emissionImageInfo.imageView = em->textureImageView;
        emissionImageInfo.sampler = em->textureSampler;

        VkWriteDescriptorSet colorDescriptorWriteSet{};
        colorDescriptorWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        colorDescriptorWriteSet.dstSet = m.descriptorSet;
        colorDescriptorWriteSet.dstBinding = 0;
        colorDescriptorWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        colorDescriptorWriteSet.descriptorCount = 1;
        colorDescriptorWriteSet.pImageInfo = &colorImageInfo;

        VkWriteDescriptorSet normalDescriptorWriteSet{};
        normalDescriptorWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        normalDescriptorWriteSet.dstSet = m.descriptorSet;
        normalDescriptorWriteSet.dstBinding = 1;
        normalDescriptorWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        normalDescriptorWriteSet.descriptorCount = 1;
        normalDescriptorWriteSet.pImageInfo = &normalImageInfo;

        VkWriteDescriptorSet metallicRoughnessDescriptorWriteSet{};
        metallicRoughnessDescriptorWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        metallicRoughnessDescriptorWriteSet.dstSet = m.descriptorSet;
        metallicRoughnessDescriptorWriteSet.dstBinding = 2;
        metallicRoughnessDescriptorWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        metallicRoughnessDescriptorWriteSet.descriptorCount = 1;
        metallicRoughnessDescriptorWriteSet.pImageInfo = &mrImageInfo;

        VkWriteDescriptorSet aoDescriptorWriteSet{};
        aoDescriptorWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        aoDescriptorWriteSet.dstSet = m.descriptorSet;
        aoDescriptorWriteSet.dstBinding = 3;
        aoDescriptorWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        aoDescriptorWriteSet.descriptorCount = 1;
        aoDescriptorWriteSet.pImageInfo = &aoImageInfo;

        VkWriteDescriptorSet emDescriptorWriteSet{};
        emDescriptorWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        emDescriptorWriteSet.dstSet = m.descriptorSet;
        emDescriptorWriteSet.dstBinding = 4;
        emDescriptorWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        emDescriptorWriteSet.descriptorCount = 1;
        emDescriptorWriteSet.pImageInfo = &emissionImageInfo;

        std::vector<VkWriteDescriptorSet> descriptorWriteSets = { colorDescriptorWriteSet, normalDescriptorWriteSet, metallicRoughnessDescriptorWriteSet, aoDescriptorWriteSet, emDescriptorWriteSet };

        vkUpdateDescriptorSets(this->vkR->device, static_cast<uint32_t>(descriptorWriteSets.size()), descriptorWriteSets.data(), 0, nullptr);
    }
}





























///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
TEXTURE HELPER PRIVATE METHODS
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void TextureHelper::transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
    VkCommandBuffer commandBuffer = this->vkR->beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else {
        std::cout << "unsupp layout trans" << std::endl;
        std::_Xinvalid_argument("Unsupported layout transition!");
    }

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    this->vkR->endSingleTimeCommands(commandBuffer);

    std::cout << "transitioned image layout" << std::endl;
}

void TextureHelper::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = this->vkR->beginSingleTimeCommands();
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = { 0, 0, 0 };
    region.imageExtent = { width, height, 1 };

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    this->vkR->endSingleTimeCommands(commandBuffer);

    std::cout << "created texture image" << std::endl;
}

void TextureHelper::createTextureImages() {
    unsigned char* buff = nullptr;
    VkDeviceSize buffSize = 0;
    bool deleteBuff = false;
    int texWidth, texHeight, texChannels;
    tinygltf::Image& curImage = in->images[0];
    VkDeviceSize imageSize;
    bool dummy = false;
    stbi_uc* pixels = nullptr;
    switch (this->i) {
    case -1:
        pixels = stbi_load("C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/dummyAO.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        imageSize = texWidth * texHeight * 4;
        this->mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        dummy = true;
        break;
    case -2:
        pixels = stbi_load("C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/dummyMetallicRoughness.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        imageSize = texWidth * texHeight * 4;
        this->mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        dummy = true;
        break;
    case -3:
        pixels = stbi_load("C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/dummyNormal.png", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        imageSize = texWidth * texHeight * 4;
        this->mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        dummy = true;
        break;
    default:
        curImage = in->images[i];

        // load images
        if (curImage.component == 3) {
            buffSize = curImage.width * curImage.height * 4;
            buff = new unsigned char[buffSize];
            unsigned char* rgba = buff;
            unsigned char* rgb = &curImage.image[0];
            for (size_t j = 0; j < curImage.width * curImage.height; j++) {
                memcpy(rgba, rgb, sizeof(unsigned char) * 3);
                rgba += 4;
                rgb += 3;
            }
            deleteBuff = true;
        }
        else {
            buff = &curImage.image[0];
            buffSize = curImage.image.size();
        }

        this->mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(curImage.width, curImage.height)))) + 1;
    }

    if (dummy) {
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        this->vkR->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(this->vkR->device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(this->vkR->device, stagingBufferMemory);

        stbi_image_free(pixels);

        this->vkR->createImage(texWidth, texHeight, this->mipLevels, VK_SAMPLE_COUNT_1_BIT, this->imageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
        transitionImageLayout(textureImage, this->imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, this->mipLevels);
        copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

        vkDestroyBuffer(this->vkR->device, stagingBuffer, nullptr);
        vkFreeMemory(this->vkR->device, stagingBufferMemory, nullptr);

        generateMipmaps(textureImage, this->imageFormat, curImage.width, curImage.height, this->mipLevels);
    }
    else {
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        this->vkR->createBuffer(buffSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        // could mess up since buffer no longer requires same memory // DELETE IF WORKING
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(this->vkR->device, stagingBuffer, &memRequirements);

        void* data;
        vkMapMemory(this->vkR->device, stagingBufferMemory, 0, memRequirements.size, 0, &data);
        memcpy(data, buff, buffSize);
        vkUnmapMemory(this->vkR->device, stagingBufferMemory);

        this->vkR->createImage(curImage.width, curImage.height, this->mipLevels, VK_SAMPLE_COUNT_1_BIT, this->imageFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
        transitionImageLayout(textureImage, this->imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, this->mipLevels);

        copyBufferToImage(stagingBuffer, textureImage, curImage.width, curImage.height);

        vkDestroyBuffer(this->vkR->device, stagingBuffer, nullptr);
        vkFreeMemory(this->vkR->device, stagingBufferMemory, nullptr);

        generateMipmaps(textureImage, this->imageFormat, curImage.width, curImage.height, this->mipLevels);

        if (deleteBuff) {
            delete[] buff;
        }
    }
}


void TextureHelper::generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
    // Check if image format supports linear blitting
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(this->vkR->GPU, imageFormat, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    VkCommandBuffer commandBuffer = this->vkR->beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = { 0, 0, 0 };
        blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = { 0, 0, 0 };
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    this->vkR->endSingleTimeCommands(commandBuffer);

    std::cout << "pipeline barrier for mipmaps" << std::endl << std::endl;
}

void TextureHelper::createTextureImageView(VkFormat f) {
    textureImageView = this->vkR->createImageView(textureImage, this->imageFormat, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
}

void TextureHelper::createTextureImageSampler() {
    VkSamplerCreateInfo samplerCInfo{};
    samplerCInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCInfo.magFilter = VK_FILTER_LINEAR;
    samplerCInfo.minFilter = VK_FILTER_LINEAR;
    samplerCInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCInfo.anisotropyEnable = VK_TRUE;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(this->vkR->GPU, &properties);
    samplerCInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerCInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerCInfo.unnormalizedCoordinates = VK_FALSE;
    samplerCInfo.compareEnable = VK_FALSE;
    samplerCInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerCInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCInfo.mipLodBias = 0.0f;
    samplerCInfo.minLod = 0.0f;
    samplerCInfo.maxLod = 0.0f;

    if (vkCreateSampler(this->vkR->device, &samplerCInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create the texture sampler!");
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
TEXTURE HELPER PUBLIC METHODS
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

TextureHelper::TextureHelper(tinygltf::Model& in, int i) {
    this->in = &in;
    this->i = i;
    this->vkR = nullptr;

    this->descriptorSet = VK_NULL_HANDLE;
    this->textureImage = VK_NULL_HANDLE;
    this->textureImageView = VK_NULL_HANDLE;
    this->textureImageMemory = VK_NULL_HANDLE;
    this->textureSampler = VK_NULL_HANDLE;
}

void TextureHelper::load() {
    createTextureImages();
    createTextureImageView();
    createTextureImageSampler();
}