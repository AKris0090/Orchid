#include "ModelHelper.h"
#include "VulkanRenderer.h"

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::string model_paths[] = {
    //"VikingRoom/OBJ.obj",
    "GSX/Srad 750.obj"
};

// Create vulkan rendering pipeline
VulkanRenderer vkR;

std::vector<ModelHelper*> models;

static void check_vk_result(VkResult err)
{
    if (err == 0)
        return;
    fprintf(stderr, "[vulkan] Error: VkResult = %d\n", err);
    if (err < 0)
        abort();
}


void setupImGUI(SDL_Window* window) {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForVulkan(window);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = vkR.instance;
    init_info.PhysicalDevice = vkR.GPU;
    init_info.Device = vkR.device;
    init_info.QueueFamily = vkR.QFIndices.graphicsFamily.value();
    init_info.Queue = vkR.graphicsQueue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = vkR.descriptorPool;
    init_info.MSAASamples = vkR.msaaSamples;
    init_info.MinImageCount = 2;
    init_info.ImageCount = 2;
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info, vkR.renderPass);
}

int main(int argc, char* argv[]) {
    // Startup the video feed
    SDL_Init(SDL_INIT_VIDEO);

    // Create the SDL Window and open
    SDL_Window* window = SDL_CreateWindow("Vulkan Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

    // Create the renderer for the window
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

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

    vkR.numModels = (sizeof(model_paths) / sizeof(*model_paths));
    std::cout << "loading: " << vkR.numModels << " models" <<std::endl << std::endl;

    for (std::string s : model_paths) {
        ModelHelper* mod = new ModelHelper(s, &vkR);
        models.push_back(mod);
        mod->load();

        std::cout << "\nloaded model: " << s << ": " << mod->model.totalVertices << " vertices, " << mod->model.totalIndices << " indices\n" << std::endl;
    }

    vkR.createUniformBuffers();

    vkR.createDescriptorPool();

    vkR.createDescriptorSets();

    std::cout << "\ncreated descriptor sets" << std::endl;

    vkR.createCommandBuffers(MAX_FRAMES_IN_FLIGHT);

    std::cout << "created commaned buffers" << std::endl;

    vkR.createSemaphores(MAX_FRAMES_IN_FLIGHT);

    std::cout << "created semaphores \n" << std::endl;

    setupImGUI(window);

    bool running = true;
    const Uint8* keystates = SDL_GetKeyboardState(NULL);
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            //if (keystates[SDL_SCANCODE_W]) {
            //    vkR.camY += 0.1;
            //}
            //else if (keystates[SDL_SCANCODE_A]) {
            //    vkR.camX += 0.1;
            //}
            //else if (keystates[SDL_SCANCODE_S]) {
            //    vkR.camY -= 0.1;
            //}
            //else if (keystates[SDL_SCANCODE_D]) {
            //    vkR.camX -= 0.1;
            //}
            //else if (keystates[SDL_SCANCODE_E]) {
            //    vkR.camZ += 0.1;
            //}
            //else if (keystates[SDL_SCANCODE_Q]) {
            //    vkR.camZ -= 0.1;
            //}
            ImGui_ImplSDL2_ProcessEvent(&event);
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_WINDOWEVENT_RESIZED:
                    vkR.frBuffResized = true;
                default:
                    break;
            }
        }
        vkR.drawNewFrame(window, models, MAX_FRAMES_IN_FLIGHT);

        // (After event loop)
        // Start the Dear ImGui frame
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        ImGui::ShowDemoWindow(); // Show demo window! :)

        // Rendering

        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), vkR.commandBuffers[vkR.currentFrame]);

        vkR.postDrawEndCommandBuffer(vkR.commandBuffers[vkR.currentFrame], window, models, MAX_FRAMES_IN_FLIGHT);
    }
    vkDeviceWaitIdle(vkR.device);

    // Cleanup after looping before exiting program
    //vkR.freeEverything(MAX_FRAMES_IN_FLIGHT, models);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyWindowSurface(window);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}