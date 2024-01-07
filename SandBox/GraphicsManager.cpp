#include "GraphicsManager.h"

GraphicsManager::GraphicsManager(std::string modelStr[], std::string textureStr[], int numModels, int numTextures) {
    this->model_paths = modelStr;
    this->texture_paths = textureStr;
    this->numModels = numModels;
    this->numTextures = numTextures;
    this->renderer = nullptr;
    this->window = nullptr;
    this->vkR = nullptr;
}

void GraphicsManager::setupImGUI() {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Platform/Renderer backends
    ImGui_ImplSDL2_InitForVulkan(window);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = this->vkR->instance;
    init_info.PhysicalDevice = this->vkR->GPU;
    init_info.Device = this->vkR->device;
    init_info.QueueFamily = this->vkR->QFIndices.graphicsFamily.value();
    init_info.Queue = this->vkR->graphicsQueue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = this->vkR->descriptorPool;
    init_info.MSAASamples = this->vkR->msaaSamples;
    init_info.MinImageCount = 2;
    init_info.ImageCount = 2;
    init_info.CheckVkResultFn = this->check_vk_result;
    ImGui_ImplVulkan_Init(&init_info, this->vkR->renderPass);
}

void GraphicsManager::check_vk_result(VkResult err) {
    if (err == 0)
        return;
    fprintf(stderr, "[vulkan] Error: this->vkResult = %d\n", err);
    if (err < 0)
        abort();
}

void GraphicsManager::startVulkan() {
    this->vkR = new VulkanRenderer(this->numModels, this->numTextures);

    this->vkR->instance = this->vkR->createVulkanInstance(this->window, "Vulkan Game Engine");

    // enableValLayers check in function
    this->vkR->setupDebugMessenger(this->vkR->instance, this->vkR->debugMessenger);

    this->vkR->createSurface(this->window);
    std::cout << "created surface \n" << std::endl;

    this->vkR->pickPhysicalDevice();

    this->vkR->createLogicalDevice();

    this->vkR->createSWChain(this->window);

    this->vkR->createImageViews();

    this->vkR->createRenderPass();
    std::cout << "created render pass" << std::endl;

    this->vkR->createCommandPool();

    this->vkR->createColorResources();
    std::cout << "created color resources" << std::endl;

    this->vkR->createDepthResources();
    std::cout << "created depth resources \n" << std::endl;

    this->vkR->createFrameBuffer();
    std::cout << "created frame buffers \n" << std::endl;

    std::cout << "loading: " << this->numModels << " models" << std::endl << std::endl;

    for (int i = 0; i < this->numModels; i++) {
        std::string s = *(this->model_paths + i);
        ModelHelper* mod = new ModelHelper(s);
        mod->vkR = this->vkR;
        this->vkR->models.push_back(mod);
        mod->loadGLTF();

        std::cout << "\nloaded model: " << s << ": " << mod->model.totalVertices << " vertices, " << mod->model.totalIndices << " indices\n" << std::endl;
    }

    this->vkR->camera.setVelocity(glm::vec3(0.0f));
    this->vkR->camera.setPosition(glm::vec3(1.0f, 0.0f, 0.0f));
    this->vkR->camera.setPitchYaw(0.0f, 0.0f);

    //ADDITIONAL CHANGES /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    this->vkR->numMats = static_cast<uint32_t>(this->vkR->models[0]->mats.size());
    this->vkR->numImages = static_cast<uint32_t>(this->vkR->models[0]->images.size());
    //models[0]->textureIndex = 1;
    //models[1]->textureIndex = 0;
    //models[0]->model.transform = glm::rotate(glm::mat4(1.0), glm::radians(90.0f), glm::vec3(-1.0, 0.0, 0.0));
    //models[0]->model.transform = glm::rotate(models[0]->model.transform, glm::radians(45.0f), glm::vec3(0.0, 1.0, -1.0));
    //models[0]->model.transform = glm::translate(models[0]->model.transform, glm::vec3(0.0, 1.35, 0.75));

    //models[1]->model.transform = glm::translate(glm::mat4(1.0), glm::vec3(0.0, 0.0, 1.35));
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    this->vkR->createUniformBuffers();

    this->vkR->createDescriptorPool();

    this->vkR->createDescriptorSetLayout();
    std::cout << "created desc set layout" << std::endl;

    this->vkR->createDescriptorSets();

    for (int i = 0; i < this->numModels; i++) {
        ModelHelper* m = this->vkR->models[i];
        m->createDescriptors();
        this->vkR->createGraphicsPipeline(m);
        std::cout << "created graphics pipeline" << std::endl;
    }
    std::cout << "\ncreated descriptor sets" << std::endl;

    /*std::cout << "loading: " << this->numTextures << " textures" << std::endl << std::endl;*/

    //for (int i = 0; i < this->numTextures; i++) {
    //    std::string s = *(this->texture_paths + i);
    //    TextureHelper* tex = new TextureHelper(s, &this->vkR);
    //    textures.push_back(tex);
    //    tex->load();

    //    std::cout << "\nloaded texture: " << s << ": " << std::endl;
    //}

    this->vkR->createCommandBuffers(MAX_FRAMES_IN_FLIGHT);
    std::cout << "created commaned buffers" << std::endl;

    this->vkR->createSemaphores(MAX_FRAMES_IN_FLIGHT);
    std::cout << "created semaphores \n" << std::endl;
}

void GraphicsManager::startSDL() {
    // Startup the video feed
    SDL_Init(SDL_INIT_VIDEO);

    // Create the SDL Window and open
    this->window = SDL_CreateWindow("Vulkan Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

    // Create the renderer for the window
    this->renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRelativeMouseMode(SDL_TRUE);
}





void GraphicsManager::loopUpdate() {
    // (After event loop)
    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Var Editor");
    ImGui::ColorEdit3("clear color", (float*)&this->vkR->clearValue.color);

    ImGui::Checkbox("rotate", &this->vkR->rotate);

    ImGui::End();
    
    this->vkR->drawNewFrame(window, MAX_FRAMES_IN_FLIGHT);

    // Rendering
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), this->vkR->commandBuffers[this->vkR->currentFrame]);


    this->vkR->postDrawEndCommandBuffer(this->vkR->commandBuffers[this->vkR->currentFrame], window, MAX_FRAMES_IN_FLIGHT);

    vkDeviceWaitIdle(this->vkR->device);
}

void GraphicsManager::setup() {
    this->startSDL();
    this->startVulkan();
    this->setupImGUI();
}

void GraphicsManager::shutDown() {
    // Cleanup after looping before exiting program
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyWindowSurface(this->window);
    SDL_DestroyWindow(this->window);
    SDL_Quit();

}