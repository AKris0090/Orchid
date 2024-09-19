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
    init_info.Queue = pVkR_->graphicsQueue_;
    init_info.DescriptorPool = pVkR_->descriptorPool_;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info, pVkR_->toneMapPass_);
}

void GraphicsManager::startSDL() {
    SDL_Init(SDL_INIT_VIDEO);

    pWindow_ = SDL_CreateWindow("Orchid Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    pRenderer_ = SDL_CreateRenderer(pWindow_, -1, SDL_RENDERER_ACCELERATED);

    SDL_SetRelativeMouseMode(SDL_TRUE);
}

void GraphicsManager::setup() {
    startSDL();
    startVulkan();
    setupImGUI();
}

void GraphicsManager::shutDown() {
    vkDeviceWaitIdle(pVkR_->device_);

    //pVkR_->shutdown();
    
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyWindowSurface(pWindow_);
    SDL_DestroyWindow(pWindow_);
    SDL_Quit();
}

void GraphicsManager::imGUIUpdate() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    ImGui::Begin("FPS MENU");
    auto framesPerSecond = 1.0f / Time::getDeltaTime();
    ImGui::Text("rfps: %.0f", framesPerSecond);
    ImGui::Text("  ft: %.2f ms", Time::getDeltaTime() * 1000.0f);
}

using namespace std::literals;

void GraphicsManager::startVulkan() {
    pVkR_->pDevHelper_ = new DeviceHelper();
    std::cout << "created: device helper" << std::endl;

    pVkR_->camera_.update();

    pVkR_->instance_ = pVkR_->createVulkanInstance(pWindow_, "Vulkan Game Engine");
    std::cout << "created: vulkan instance" << std::endl;

    pVkR_->setupDebugMessenger(pVkR_->instance_, pVkR_->debugMessenger_);

    pVkR_->createSurface(pWindow_);
    std::cout << "created surface" << std::endl;

    pVkR_->pickPhysicalDevice();
    pVkR_->pDevHelper_->gpu_ = pVkR_->GPU_;
    std::cout << "chose physical device" << std::endl;

    pVkR_->createLogicalDevice();
    pVkR_->pDevHelper_->device_ = pVkR_->device_;
    pVkR_->pDevHelper_->graphicsQueue_ = pVkR_->graphicsQueue_;
    std::cout << "created logical device" << std::endl;

    pVkR_->createSWChain(pWindow_);
    std::cout << "chreated swap chain" << std::endl;

    pVkR_->createImageViews();
    std::cout << "created swap chain image views" << std::endl;

    pVkR_->createRenderPass();
    std::cout << "created render pass" << std::endl;

    pVkR_->createCommandPool();
    pVkR_->pDevHelper_->commandPool_ = pVkR_->commandPool_;
    std::cout << "created command pool" << std::endl;

    pVkR_->createColorResources();
    std::cout << "created color resources" << std::endl;

    pVkR_->createDepthResources();
    std::cout << "created depth resources" << std::endl;

    pVkR_->createFrameBuffer();
    std::cout << "created frame buffers" << std::endl;

    pVkR_->createDescriptorPool();
    pVkR_->pDevHelper_->descPool_ = pVkR_->descriptorPool_;
    std::cout << "created descriptor pool" << std::endl;

    pVkR_->pDirectionalLight_->setup(pVkR_->pDevHelper_, &(pVkR_->graphicsQueue_), &(pVkR_->commandPool_), pVkR_->SWChainExtent_.width, pVkR_->SWChainExtent_.height);

    pVkR_->camera_.setProjectionMatrix();
    pVkR_->pDirectionalLight_->genShadowMap(&(pVkR_->camera_), &(pVkR_->modelMatrixSetLayout_->layout), MAX_FRAMES_IN_FLIGHT);

    std::cout << std::endl << "generated Shadow Map" << std::endl;

    pVkR_->createUniformBuffers();
    std::cout << "created uniform buffers" << std::endl;

    pVkR_->createDescriptorSetLayout();
    std::cout << "created desc set layout" << std::endl;

    pVkR_->pDirectionalLight_->createPipeline(pVkR_->modelMatrixSetLayout_);

    std::cout << "loading skybox\n" << std::endl;

    uint32_t globalVertexOffset = 6;
    uint32_t globalIndexOffset = 6;

    pVkR_->vertices_ = { Vertex(glm::vec2(-1.0f, -1.0f), glm::vec2(0.0f, 0.0f)),
                       Vertex(glm::vec2(-1.0f, 1.0f), glm::vec2(0.0f, 1.0f)),
                       Vertex(glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 1.0f)),
                       Vertex(glm::vec2(-1.0f, -1.0f), glm::vec2(0.0f, 0.0f)),
                       Vertex(glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 1.0f)),
                       Vertex(glm::vec2(1.0f, -1.0f), glm::vec2(1.0f, 0.0f))
    };

    pVkR_->indices_ = { 0, 1, 2, 3, 4, 5 };

    pVkR_->pSkyBox_ = new Skybox(skyboxModelPath_, skyboxTexturePaths_, pVkR_->pDevHelper_, globalVertexOffset, globalIndexOffset);
    pVkR_->createSkyBoxPipeline();

    for (int i = 0; i < pVkR_->pSkyBox_->pSkyBoxModel_->totalVertices_; i++) {
        pVkR_->vertices_.push_back(pVkR_->pSkyBox_->pSkyBoxModel_->vertices_[i]);
        globalVertexOffset++;
    }

    for (int i = 0; i < pVkR_->pSkyBox_->pSkyBoxModel_->totalIndices_; i++) {
        pVkR_->indices_.push_back(pVkR_->pSkyBox_->pSkyBoxModel_->indices_[i]);
        globalIndexOffset++;
    }

    std::cout << "DONE loading skybox\n" << std::endl;

    std::cout << std::endl << "loading: " << numModels_ << " models" << std::endl << std::endl;

    for (std::string s : pStaticModelPaths_) {
        GameObject* newGO = new GameObject();
        GLTFObj* mod = new GLTFObj(s, pVkR_->pDevHelper_, globalVertexOffset, globalIndexOffset);
        newGO->setGLTFObj(mod);
        gameObjects.push_back(newGO);

        pVkR_->numMats_ += static_cast<uint32_t>(mod->mats_.size());
        pVkR_->numImages_ += static_cast<uint32_t>(mod->images_.size());

        pVkR_->vertices_.insert(pVkR_->vertices_.end(), mod->vertices_.begin(), mod->vertices_.end());
        pVkR_->indices_.insert(pVkR_->indices_.end(), mod->indices_.begin(), mod->indices_.end());

        mod->indices_.clear();
        mod->indices_.shrink_to_fit();
        mod->vertices_.clear();
        mod->vertices_.shrink_to_fit();

        globalVertexOffset = pVkR_->vertices_.size();
        globalIndexOffset = pVkR_->indices_.size();

        newGO->isOutline = false;

        std::cout << "\nloaded model: " << s << ": " << mod->totalVertices_ << " vertices, " << mod->totalIndices_ << " indices\n" << std::endl;
    }

    uint32_t globalSkinMatrixOffset = 0;

    for (std::string s : pAnimatedModelPaths_) {
        AnimatedGameObject* newAnimGO = new AnimatedGameObject(pVkR_->pDevHelper_);
        AnimatedGLTFObj* mod = new AnimatedGLTFObj(s, pVkR_->pDevHelper_, globalVertexOffset, globalIndexOffset);
        newAnimGO->setAnimatedGLTFObj(mod);
        animatedObjects.push_back(newAnimGO);

        pVkR_->numMats_ += static_cast<uint32_t>(mod->mats_.size());
        pVkR_->numImages_ += static_cast<uint32_t>(mod->images_.size());

        newAnimGO->basePoseVertices_.insert(newAnimGO->basePoseVertices_.end(), mod->vertices_.begin(), mod->vertices_.end());

        pVkR_->vertices_.insert(pVkR_->vertices_.end(), mod->vertices_.begin(), mod->vertices_.end());
        pVkR_->indices_.insert(pVkR_->indices_.end(), mod->indices_.begin(), mod->indices_.end());

        mod->globalSkinningMatrixOffset = globalSkinMatrixOffset;

        for (auto& skin : mod->skins_) {
            for (glm::mat4& matrix : *(skin.finalJointMatrices)) {
                pVkR_->inverseBindMatrices.push_back(matrix);
                globalSkinMatrixOffset++;
                newAnimGO->numInverseBindMatrices++;
            }
        }

        newAnimGO->isOutline = true;
        newAnimGO->smoothDuration = 150ms;
        newAnimGO->smoothAmount = FLT_MAX;
        newAnimGO->src = new std::vector<AnimatedGameObject::secondaryTransform>(newAnimGO->renderTarget->walkAnim.numChannels);
        newAnimGO->dst = new std::vector<AnimatedGameObject::secondaryTransform>(newAnimGO->renderTarget->walkAnim.numChannels);

        globalVertexOffset = pVkR_->vertices_.size();
        globalIndexOffset = pVkR_->indices_.size();

        MeshHelper::createVertexBuffer(pVkR_->pDevHelper_, newAnimGO->basePoseVertices_, newAnimGO->vertexBuffer_, newAnimGO->vertexBufferMemory_);

        std::cout << "\nloaded model: " << s << ": " << mod->totalVertices_ << " vertices, " << mod->totalIndices_ << " indices\n" << std::endl;
    }

    // link renderable objects to member game objects
    pVkR_->gameObjects = &gameObjects;
    pVkR_->animatedObjects = &animatedObjects;

    pVkR_->createVertexBuffer();
    pVkR_->createIndexBuffer();

    pVkR_->pDevHelper_->texDescSetLayout_ = pVkR_->textureDescriptorSetLayout_->layout;

    pVkR_->createDescriptorSets();
    std::cout << "created desc sets" << std::endl << std::endl;

    pVkR_->brdfLut = new BRDFLut(pVkR_->pDevHelper_);
    std::cout << "generated BRDFLUT" << std::endl;

    pVkR_->irCube = new IrradianceCube(pVkR_->pDevHelper_, pVkR_->pSkyBox_, pVkR_->vertexBuffer_, pVkR_->indexBuffer_);
    std::cout << std::endl << "generated IrradianceCube" << std::endl;

    pVkR_->prefEMap = new PrefilteredEnvMap(pVkR_->pDevHelper_, pVkR_->pSkyBox_, pVkR_->vertexBuffer_, pVkR_->indexBuffer_);

    std::cout << std::endl << "generated Prefiltered Environment Map" << std::endl;

    for (GameObject* gO : gameObjects) {
        gO->renderTarget->createDescriptors();
    }

    for (AnimatedGameObject* gO : animatedObjects) {
        gO->renderTarget->createDescriptors();
    }

    pVkR_->updateGeneratedImageDescriptorSets();
    std::cout << "\ncreated descriptor sets" << std::endl;

    pVkR_->createGraphicsPipeline();
    std::cout << "created material graphics pipeline" << std::endl;

    pVkR_->createDepthPipeline();
    std::cout << "created depth pipeline" << std::endl;

    pVkR_->createToonPipeline();
    std::cout << "created cartoon graphics pipeline" << std::endl;

    pVkR_->createOutlinePipeline();
    std::cout << "created outline pipeline" << std::endl;

    pVkR_->createToneMappingPipeline();
    std::cout << "created tonemapping pipeline" << std::endl;

    pVkR_->createCommandBuffers(MAX_FRAMES_IN_FLIGHT);
    std::cout << "created commaned buffers" << std::endl;

    pVkR_->createSemaphores(MAX_FRAMES_IN_FLIGHT);
    std::cout << "created semaphores \n" << std::endl;

    pVkR_->separateDrawCalls();

    pVkR_->setupCompute(MAX_FRAMES_IN_FLIGHT);

    pVkR_->bloomHelper = new BloomHelper(pVkR_->pDevHelper_);
        
    pVkR_->bloomHelper->setupBloom(&(pVkR_->bloomResolveImage_), &(pVkR_->bloomResolveImageView_), VK_FORMAT_R16G16B16A16_SFLOAT, pVkR_->SWChainExtent_);
    std::cout << "setup bloom" << std::endl;

    return;
}

void GraphicsManager::loopUpdate() {
    imGUIUpdate();

    pVkR_->drawNewFrame(pWindow_, MAX_FRAMES_IN_FLIGHT);

    // IMGUI Rendering
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), pVkR_->commandBuffers_[pVkR_->currentFrame_]);

    pVkR_->postDrawEndCommandBuffer(pVkR_->commandBuffers_[pVkR_->currentFrame_], pWindow_, MAX_FRAMES_IN_FLIGHT);

    frameCount++;
}

GraphicsManager::GraphicsManager(std::vector<std::string> staticModelPaths, std::vector<std::string> animatedModelPaths, std::string skyboxModelPath, std::vector<std::string> skyboxTexturePaths, float windowWidth, float windowHeight) {
    this->pStaticModelPaths_ = staticModelPaths;
    this->pAnimatedModelPaths_ = animatedModelPaths;
    this->skyboxTexturePaths_ = skyboxTexturePaths;
    this->skyboxModelPath_ = skyboxModelPath;
    this->numModels_ = staticModelPaths.size();
    this->numAnimatedModels_ = animatedModelPaths.size();
    this->numTextures_ = 0;
    this->pRenderer_ = nullptr;
    this->pWindow_ = nullptr;
    this->pVkR_ = nullptr;
    this->windowWidth = windowWidth;
    this->windowHeight = windowHeight;
    this->frameCount = 0;
}