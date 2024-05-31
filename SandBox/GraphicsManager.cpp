#include "GraphicsManager.h"

void GraphicsManager::check_vk_result(VkResult err) {
    if (err == 0)
        return;
    fprintf(stderr, "[vulkan] Error: pVkResult = %d\n", err);
    if (err < 0)
        abort();
}

void GraphicsManager::setupImGUI() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui_ImplSDL2_InitForVulkan(pWindow_);
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = pVkR_->instance_;
    init_info.PhysicalDevice = pVkR_->GPU_;
    init_info.Device = pVkR_->device_;
    init_info.QueueFamily = pVkR_->QFIndices_.graphicsFamily.value();
    init_info.Queue = pVkR_->graphicsQueue_;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = pVkR_->descriptorPool_;
    init_info.MSAASamples = pVkR_->msaaSamples_;
    init_info.MinImageCount = 2;
    init_info.ImageCount = 2;
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info, pVkR_->renderPass_);
}

void GraphicsManager::startSDL() {
    // Startup the video feed
    SDL_Init(SDL_INIT_VIDEO);

    // Create the SDL Window and open
    pWindow_ = SDL_CreateWindow("Vulkan Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 1280, 720, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

    // Create the renderer for the window
    pRenderer_ = SDL_CreateRenderer(pWindow_, -1, SDL_RENDERER_ACCELERATED);
    SDL_SetRelativeMouseMode(SDL_TRUE);
}

void GraphicsManager::setup() {
    startSDL();
    startVulkan();
    setupImGUI();
}

void GraphicsManager::shutDown() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyWindowSurface(pWindow_);
    SDL_DestroyWindow(pWindow_);
    SDL_Quit();
}

void GraphicsManager::imGUIUpdate() {
    // Start the Dear ImGui frame
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("Var Editor");
    ImGui::DragFloat("lightX", &pVkR_->pLightPos_->x);
    ImGui::DragFloat("lightY", &pVkR_->pLightPos_->y);
    ImGui::DragFloat("lightZ", &pVkR_->pLightPos_->z);
    ImGui::DragFloat("near plane", &pVkR_->shadowMap->zNear);
    ImGui::DragFloat("far plane", &pVkR_->shadowMap->zFar);
    ImGui::SliderFloat("X", &pVkR_->camera_.transform.position.x, -50.0f, 50.0f);
    ImGui::SliderFloat("Y", &pVkR_->camera_.transform.position.y, -50.0f, 50.0f);
    ImGui::SliderFloat("Z", &pVkR_->camera_.transform.position.z, -50.0f, 50.0f);
    ImGui::ColorEdit3("clear color", (float*)&pVkR_->clearValue_.color);
    ImGui::DragFloat("depthBias", &pVkR_->depthBias);
    ImGui::Checkbox("rotate", &pVkR_->rotate_);
    ImGui::End();

    //ImGui::Begin("Viewport");
    //ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
    //ImGui::Image(m_Dset, ImVec2{ viewportPanelSize.x, viewportPanelSize.y });
    //ImGui::End();
}

void GraphicsManager::startVulkan() {
    for (int i = 0; i < numModels_; i++) {
        gameObjects.push_back(new GameObject());
    }

    pVkR_ = new VulkanRenderer(numModels_);
    pVkR_->pDevHelper_ = new DeviceHelper();
    pVkR_->pLightPos_ = new glm::vec4(20.0f, 40.0f, 8.0f, 1.0f);
    pVkR_->depthBias = 0.0003f;
    pVkR_->camera_.setNearPlane(0.0001f);
    pVkR_->camera_.setFarPlane(10000.0f);
    pVkR_->camera_.setFOV(glm::radians(75.0f));

    pVkR_->camera_.update();

    pVkR_->instance_ = pVkR_->createVulkanInstance(pWindow_, "Vulkan Game Engine");

    // enableValLayers check in function
    pVkR_->setupDebugMessenger(pVkR_->instance_, pVkR_->debugMessenger_);

    pVkR_->createSurface(pWindow_);
    std::cout << "created surface" << std::endl;

    pVkR_->pickPhysicalDevice();
    pVkR_->pDevHelper_->setPhysicalDevice(pVkR_->GPU_);
    std::cout << "chose physical device" << std::endl;

    pVkR_->createLogicalDevice();
    pVkR_->pDevHelper_->setDevice(pVkR_->device_);
    pVkR_->pDevHelper_->setGraphicsQueue(pVkR_->graphicsQueue_);
    std::cout << "created logical device" << std::endl;

    pVkR_->createSWChain(pWindow_);
    std::cout << "chreated swap chain" << std::endl;

    pVkR_->camera_.setAspectRatio((float)pVkR_->SWChainExtent_.width / (float)pVkR_->SWChainExtent_.height);

    pVkR_->createImageViews();
    std::cout << "created swap chain image views" << std::endl;

    pVkR_->createRenderPass();
    std::cout << "created render pass" << std::endl;

    pVkR_->createCommandPool();
    pVkR_->pDevHelper_->setCommandPool(pVkR_->commandPool_);
    std::cout << "created command pool" << std::endl;

    pVkR_->createColorResources();
    std::cout << "created color resources" << std::endl;

    pVkR_->createDepthResources();
    std::cout << "created depth resources" << std::endl;

    pVkR_->createFrameBuffer();
    std::cout << "created frame buffers" << std::endl;

    std::cout << std::endl << "loading: " << numModels_ << " models" << std::endl << std::endl;
    for (int i = 0; i < numModels_; i++) {
        std::string s = *(pModelPaths_ + i);
        GLTFObj* mod = new GLTFObj(s, pVkR_->pDevHelper_);
        pVkR_->pModels_.push_back(mod);
        mod->loadGLTF();

        pVkR_->numMats_ += static_cast<uint32_t>(mod->getMeshHelper()->mats_.size());
        pVkR_->numImages_ = static_cast<uint32_t>(mod->getMeshHelper()->images_.size());

        std::cout << "\nloaded model: " << s << ": " << mod->getTotalVertices() << " vertices, " << mod->getTotalIndices() << " indices\n" << std::endl;

        gameObjects[i]->setGLTFObj(mod);
    }

    animatedObjects.push_back(new AnimatedGameObject());
    animatedObjects[0]->setAnimatedGLTFObj(new AnimatedGLTFObj("C://Users//arjoo//OneDrive//Documents//GameProjects//SndBx//SandBox//emily//Emily_Walk.glb", pVkR_->pDevHelper_));
    animatedObjects[0]->renderTarget->loadGLTF();
    numAnimated = 1;
    pVkR_->animatedObjects = animatedObjects;

    pVkR_->createDescriptorPool();
    pVkR_->pDevHelper_->setDescriptorPool(pVkR_->descriptorPool_);
    std::cout << "created descriptor pool" << std::endl;

    pVkR_->shadowMap = new DirectionalLight(pVkR_->pDevHelper_, &(pVkR_->graphicsQueue_), &(pVkR_->commandPool_), pVkR_->pLightPos_, pVkR_->pModels_, pVkR_->numModels_, pVkR_->SWChainExtent_.width, pVkR_->SWChainExtent_.height);

    glm::mat4 proj = glm::perspective(pVkR_->camera_.getFOV(), pVkR_->camera_.getAspectRatio(), pVkR_->camera_.getNearPlane(), pVkR_->camera_.getFarPlane());
    proj[1][1] *= -1;
    pVkR_->shadowMap->genShadowMap(proj, pVkR_->camera_.getViewMatrix(), pVkR_->camera_.getNearPlane(), pVkR_->camera_.getFarPlane(), pVkR_->camera_.getAspectRatio());

    std::cout << std::endl << "generated Shadow Map" << std::endl;

    pVkR_->createUniformBuffers();
    std::cout << "created uniform buffers" << std::endl;

    pVkR_->createDescriptorSetLayout();
    std::cout << "created desc set layout" << std::endl;

    pVkR_->pDevHelper_->setTextureDescSetLayout(pVkR_->textureDescriptorSetLayout_);

    pVkR_->createDescriptorSets();
    std::cout << "created desc sets" << std::endl << std::endl;

    std::cout << "loading skybox\n" << std::endl;

    pVkR_->pSkyBox_ = new Skybox(skyboxModelPath_, skyboxTexturePaths_, pVkR_->pDevHelper_);
    pVkR_->pSkyBox_->loadSkyBox();
    pVkR_->createSkyBoxPipeline();

    std::cout << "DONE loading skybox\n" << std::endl;

    pVkR_->brdfLut = new BRDFLut(pVkR_->pDevHelper_);
    pVkR_->brdfLut->genBRDFLUT();

    std::cout << "generated BRDFLUT" << std::endl;

    pVkR_->irCube = new IrradianceCube(pVkR_->pDevHelper_, &(pVkR_->graphicsQueue_), &(pVkR_->commandPool_), pVkR_->pSkyBox_);
    pVkR_->irCube->geniRCube();

    std::cout << std::endl << "generated IrradianceCube" << std::endl;

    pVkR_->prefEMap = new PrefilteredEnvMap(pVkR_->pDevHelper_, &(pVkR_->graphicsQueue_), &(pVkR_->commandPool_), pVkR_->pSkyBox_);
    pVkR_->prefEMap->genprefEMap();

    std::cout << std::endl << "generated Prefiltered Environment Map" << std::endl;

    for (int i = 0; i < numModels_; i++) {
        GLTFObj* gltfO = pVkR_->pModels_[i];
        gltfO->createDescriptors();
        std::cout << "created material graphics pipeline" << std::endl;
    }

    pVkR_->animatedObjects[0]->renderTarget->createDescriptors(pVkR_->animatedDescriptorSetLayout_);
    pVkR_->updateGeneratedImageDescriptorSets();

    pVkR_->shadowMap->createAnimatedPipeline(pVkR_->animatedDescriptorSetLayout_);


    for (int i = 0; i < numModels_; i++) {
        GLTFObj* gltfO = pVkR_->pModels_[i];
        pVkR_->createGraphicsPipeline(gltfO->getMeshHelper());
        std::cout << "created material graphics pipeline" << std::endl;
    }

    pVkR_->createAnimatedGraphicsPipeline(pVkR_->animatedObjects[0]->renderTarget->getMeshHelper());

    std::cout << "\ncreated descriptor sets" << std::endl;

    pVkR_->createCommandBuffers(MAX_FRAMES_IN_FLIGHT);
    std::cout << "created commaned buffers" << std::endl;

    pVkR_->createSemaphores(MAX_FRAMES_IN_FLIGHT);
    std::cout << "created semaphores \n" << std::endl;

    for (int i = 0; i < numModels_; i++) {
        (gameObjects)[i]->setTransform(glm::mat4(1.0f));
    }
}

void GraphicsManager::loopUpdate() {
    imGUIUpdate();

    pVkR_->drawNewFrame(pWindow_, MAX_FRAMES_IN_FLIGHT);

    // IMGUI Rendering
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), pVkR_->commandBuffers_[pVkR_->currentFrame_]);

    pVkR_->postDrawEndCommandBuffer(pVkR_->commandBuffers_[pVkR_->currentFrame_], pWindow_, MAX_FRAMES_IN_FLIGHT);

    vkDeviceWaitIdle(pVkR_->device_);
}

GraphicsManager::GraphicsManager(std::string modelStr[], std::string skyBoxModelPath, std::vector<std::string> skyBoxPaths, int numModels, int numTextures) {
    this->pModelPaths_ = modelStr;
    this->skyboxModelPath_ = skyBoxModelPath;
    this->skyboxTexturePaths_ = skyBoxPaths;
    this->numModels_ = numModels;
    this->pRenderer_ = nullptr;
    this->pWindow_ = nullptr;
    this->pVkR_ = nullptr;
}