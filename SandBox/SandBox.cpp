#include "VulkanRenderer.h"

const int MAX_FRAMES_IN_FLIGHT = 2;

glm::mat4 translationMatrix{ 1.0f };


int main(int argc, char* argv[]) {
    // Startup the video feed
    SDL_Init(SDL_INIT_VIDEO);

    // Create the SDL Window and open
    SDL_Window* window = SDL_CreateWindow("Vulkan Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_VULKAN);

    // Create the renderer for the window
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // Create vulkan rendering pipeline
    VulkanRenderer vkR;

    // Function pipeline
    glm::mat4 translationMatrix{ 1.0f };

    vkR.instance = vkR.createVulkanInstance(window, "Vulkan Game Engine");

    // enableValLayers check in function
    vkR.setupDebugMessenger(vkR.instance, vkR.debugMessenger);

    vkR.createSurface(window);

    std::cout << "created surface \n" << std::endl;

    vkR.pickPhysicalDevice();

    vkR.createLogicalDevice();

    vkR.createSWChain(window);

    vkR.createImageViews();

    vkR.createRenderPass();

    std::cout << "created render pass" << std::endl;

    vkR.createDescriptorSetLayout();

    std::cout << "created desc set layout" << std::endl;

    vkR.createGraphicsPipeline();

    std::cout << "created graphics pipeline" << std::endl;

    vkR.createCommandPool();

    vkR.createColorResources();

    std::cout << "created color resources" << std::endl;

    vkR.createDepthResources();

    std::cout << "created depth resources \n" << std::endl;

    vkR.createFrameBuffer();

    std::cout << "created frame buffers \n" << std::endl;

    vkR.createTextureImage();

    vkR.createTextureImageView();

    vkR.createTextureImageSampler();

    vkR.loadModel(translationMatrix);

    std::cout << "\nloaded model \n" << std::endl;

    vkR.createVertexBuffer();

    vkR.createIndexBuffer();

    vkR.createUniformBuffers();

    vkR.createDescriptorPool();

    vkR.createDescriptorSets();

    std::cout << "\ncreated descriptor sets" << std::endl;

    vkR.createCommandBuffers(MAX_FRAMES_IN_FLIGHT);

    std::cout << "created commaned buffers" << std::endl;

    vkR.createSemaphores(MAX_FRAMES_IN_FLIGHT);

    std::cout << "created semaphores \n" << std::endl;

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_QUIT:
                running = false;
                break;
            default:
                break;
            }
        }
        vkR.drawNewFrame(window, MAX_FRAMES_IN_FLIGHT);
    }

    // Cleanup after looping before exiting program
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}