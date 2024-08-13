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

    pWindow_ = SDL_CreateWindow("Vulkan Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
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

    pVkR_->shutdown();

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

    ImGui::Begin("FPS MENU");
    auto framesPerSecond = 1.0f / Time::getDeltaTime();
    ImGui::Text("afps: %.0f rad/s", glm::two_pi<float>() * framesPerSecond);
    ImGui::Text("dfps: %.0f �/s", glm::degrees(glm::two_pi<float>() * framesPerSecond));
    ImGui::Text("rfps: %.0f", framesPerSecond);
    ImGui::Text("rpms: %.0f", framesPerSecond * 60.0f);
    ImGui::Text("  ft: %.2f ms", Time::getDeltaTime() * 1000.0f);
    ImGui::Text("   f: %.d", frameCount);
    ImGui::End();

    ImGui::Begin("Var Editor");

    ImGui::DragFloat("playerAnimSpeed", &player->playerGameObject->smoothTime);
    ImGui::DragFloat("bloom radius", &pVkR_->bloomRadius);
    ImGui::DragFloat("specularNdotL", &pVkR_->specularCont);
    ImGui::DragFloat("specularNdotV", &pVkR_->nDotVSpec);
    ImGui::DragFloat("lightX", &pVkR_->pDirectionalLight_->transform.position.x);
    ImGui::DragFloat("lightY", &pVkR_->pDirectionalLight_->transform.position.y);
    ImGui::DragFloat("lightZ", &pVkR_->pDirectionalLight_->transform.position.z);
    ImGui::DragFloat("zNear", &pVkR_->camera_.nearPlane);
    ImGui::DragFloat("zFar", &pVkR_->camera_.farPlane);
    ImGui::DragFloat("near plane", &pVkR_->pDirectionalLight_->zNear);
    ImGui::DragFloat("far plane", &pVkR_->pDirectionalLight_->zFar);
    ImGui::DragFloat("gamma", &pVkR_->gamma_);
    ImGui::DragFloat("exposure", &pVkR_->exposure_);
    ImGui::Checkbox("tonemap", &pVkR_->applyTonemap);
    ImGui::SliderFloat("X", &pVkR_->camera_.transform.position.x, -50.0f, 50.0f);
    ImGui::SliderFloat("Y", &pVkR_->camera_.transform.position.y, -50.0f, 50.0f);
    ImGui::SliderFloat("Z", &pVkR_->camera_.transform.position.z, -50.0f, 50.0f);
    ImGui::ColorEdit3("clear color", (float*)&pVkR_->clearValue_.color);
    ImGui::DragFloat("cascadeLambda", &pVkR_->pDirectionalLight_->cascadeSplitLambda, 0.01f);
    ImGui::DragFloat("bias", &pVkR_->depthBias_);
    ImGui::DragFloat("reflectionLOD", &pVkR_->maxReflectionLOD_);
    ImGui::Checkbox("rotate", &pVkR_->rotate_);
    ImGui::End();
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
    pVkR_->pDirectionalLight_->genShadowMap(&(pVkR_->camera_));

    std::cout << std::endl << "generated Shadow Map" << std::endl;

    pVkR_->createUniformBuffers();
    std::cout << "created uniform buffers" << std::endl;

    pVkR_->createDescriptorSetLayout();
    std::cout << "created desc set layout" << std::endl;

    std::cout << "loading skybox\n" << std::endl;

    uint32_t globalVertexOffset = 0;
    uint32_t globalIndexOffset = 0;

    pVkR_->pSkyBox_ = new Skybox(skyboxModelPath_, skyboxTexturePaths_, pVkR_->pDevHelper_);
    pVkR_->pSkyBox_->loadSkyBox(globalVertexOffset, globalIndexOffset);
    pVkR_->createSkyBoxPipeline();

    for (int i = globalVertexOffset; i < pVkR_->pSkyBox_->pSkyBoxModel_->getTotalVertices(); i++) {
        pVkR_->vertices_.push_back(pVkR_->pSkyBox_->pSkyBoxModel_->pParentNodes[0]->meshPrimitives[0]->stagingVertices_[i]);
        globalVertexOffset++;
    }

    for (int i = globalIndexOffset; i < pVkR_->pSkyBox_->pSkyBoxModel_->getTotalIndices(); i++) {
        pVkR_->indices_.push_back(pVkR_->pSkyBox_->pSkyBoxModel_->pParentNodes[0]->meshPrimitives[0]->stagingIndices_[i]);
        globalIndexOffset++;
    }

    std::cout << "DONE loading skybox\n" << std::endl;

    std::cout << std::endl << "loading: " << numModels_ << " models" << std::endl << std::endl;

    for (std::string s : pStaticModelPaths_) {
        GameObject* newGO = new GameObject();
        GLTFObj* mod = new GLTFObj(s, pVkR_->pDevHelper_, globalVertexOffset, globalIndexOffset);
        mod->loadGLTF(globalVertexOffset, globalIndexOffset);
        newGO->setGLTFObj(mod);
        gameObjects.push_back(newGO);

        pVkR_->numMats_ += static_cast<uint32_t>(mod->mats_.size());
        pVkR_->numImages_ += static_cast<uint32_t>(mod->images_.size());

        mod->addVertices(&(pVkR_->vertices_));
        mod->addIndices(&(pVkR_->indices_));

        globalVertexOffset = pVkR_->vertices_.size();
        globalIndexOffset = pVkR_->indices_.size();

        newGO->isOutline = false;

        std::cout << "\nloaded model: " << s << ": " << mod->getTotalVertices() << " vertices, " << mod->getTotalIndices() << " indices\n" << std::endl;
    }

    uint32_t globalSkinMatrixOffset = 0;

    globalVertexOffset = 0;
    globalIndexOffset = 0;

    for (std::string s : pAnimatedModelPaths_) {
        AnimatedGameObject* newAnimGO = new AnimatedGameObject(pVkR_->pDevHelper_);
        AnimatedGLTFObj* mod = new AnimatedGLTFObj(s, pVkR_->pDevHelper_, globalVertexOffset, globalIndexOffset);
        mod->loadGLTF(globalVertexOffset, globalIndexOffset);
        newAnimGO->setAnimatedGLTFObj(mod);
        animatedObjects.push_back(newAnimGO);

        pVkR_->numMats_ += static_cast<uint32_t>(mod->mats_.size());
        pVkR_->numImages_ += static_cast<uint32_t>(mod->images_.size());

        mod->addVertices(&(newAnimGO->basePoseVertices_));
        mod->addIndices(&(newAnimGO->basePoseIndices_));

        mod->globalSkinningMatrixOffset = globalSkinMatrixOffset;

        for (auto& skin : mod->skins_) {
            for (glm::mat4 matrix : *(skin.finalJointMatrices)) {
                pVkR_->inverseBindMatrices.push_back(matrix);
                globalSkinMatrixOffset++;
            }
        }
        newAnimGO->isOutline = true;
        newAnimGO->smoothTime = 15.0f;
        newAnimGO->smoothDuration = 1s;


        std::cout << "\nloaded model: " << s << ": " << mod->getTotalVertices() << " vertices, " << mod->getTotalIndices() << " indices\n" << std::endl;
    }

    // link renderable objects to member game objects
    pVkR_->gameObjects = &gameObjects;
    pVkR_->animatedObjects = &animatedObjects;

    for (AnimatedGameObject* g : animatedObjects) {
        g->createVertexBuffer();
        g->createIndexBuffer();
        g->createSkinnedBuffer();
    }

    pVkR_->createVertexBuffer();
    pVkR_->createIndexBuffer();

    //pVkR_->vertices_.clear();
    //pVkR_->indices_.clear();
    //pVkR_->vertices_.shrink_to_fit();
    //pVkR_->indices_.shrink_to_fit();

    pVkR_->pDevHelper_->texDescSetLayout_ = pVkR_->textureDescriptorSetLayout_;

    pVkR_->createDescriptorSets();
    std::cout << "created desc sets" << std::endl << std::endl;

    pVkR_->brdfLut = new BRDFLut(pVkR_->pDevHelper_);
    std::cout << "generated BRDFLUT" << std::endl;

    pVkR_->irCube = new IrradianceCube(pVkR_->pDevHelper_, &(pVkR_->graphicsQueue_), &(pVkR_->commandPool_), pVkR_->pSkyBox_);
    pVkR_->irCube->geniRCube(pVkR_->vertexBuffer_, pVkR_->indexBuffer_);

    std::cout << std::endl << "generated IrradianceCube" << std::endl;

    pVkR_->prefEMap = new PrefilteredEnvMap(pVkR_->pDevHelper_, &(pVkR_->graphicsQueue_), &(pVkR_->commandPool_), pVkR_->pSkyBox_);
    pVkR_->prefEMap->genprefEMap(pVkR_->vertexBuffer_, pVkR_->indexBuffer_);

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

    pVkR_->setupCompute();

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