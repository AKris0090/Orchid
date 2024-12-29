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
    init_info.Instance = vkR_.instance_;
    init_info.PhysicalDevice = vkR_.GPU_;
    init_info.Device = vkR_.device_;
    init_info.Queue = vkR_.graphicsQueue_;
    init_info.DescriptorPool = vkR_.descriptorPool_;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.MinImageCount = 3;
    init_info.ImageCount = 3;
    init_info.CheckVkResultFn = check_vk_result;
    ImGui_ImplVulkan_Init(&init_info, vkR_.toneMapPass_);
}

void GraphicsManager::startSDL() {
    SDL_Init(SDL_INIT_VIDEO);

    pWindow_ = SDL_CreateWindow("Orchid Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, windowWidth, windowHeight, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    pRenderer_ = SDL_CreateRenderer(pWindow_, -1, SDL_RENDERER_ACCELERATED);

    SDL_SetRelativeMouseMode(SDL_TRUE);
}

void GraphicsManager::setup(std::vector<std::string>& staticModelPaths, std::vector<std::string>& animatedModelPaths, std::string& skyboxModelPath, std::vector<std::string>& skyboxTexturePaths) {
    startSDL();
    startVulkan(staticModelPaths, animatedModelPaths, skyboxModelPath, skyboxTexturePaths);
    setupImGUI();
}

void GraphicsManager::shutDown() {
    vkDeviceWaitIdle(vkR_.device_);

    //vkR_.shutdown();
    
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
    ImGui::End();

    ImGui::Begin("Var Editor");

    //ImGui::DragFloat("playerAnimSpeed", &player->playerGameObject->smoothTime);
    ImGui::DragFloat("bloom radius", &vkR_.bloomRadius);
    ImGui::DragFloat("bias0", &vkR_.biases[0]);
    ImGui::DragFloat("bias1", &vkR_.biases[1]);
    ImGui::DragFloat("bias2", &vkR_.biases[2]);
    ImGui::DragFloat("bias3", &vkR_.biases[3]);
    ImGui::DragFloat("specularNdotL", &vkR_.specularCont);
    ImGui::DragFloat("specularNdotV", &vkR_.nDotVSpec);
    ImGui::DragFloat("lightX", &vkR_.pDirectionalLight_->transform.position.x);
    ImGui::DragFloat("lightY", &vkR_.pDirectionalLight_->transform.position.y);
    ImGui::DragFloat("lightZ", &vkR_.pDirectionalLight_->transform.position.z);
    ImGui::DragFloat("zNear", &vkR_.camera_.nearPlane);
    ImGui::DragFloat("zFar", &vkR_.camera_.farPlane);
    ImGui::DragFloat("gamma", &vkR_.gamma_);
    ImGui::DragFloat("exposure", &vkR_.exposure_);
    ImGui::SliderFloat("X", &vkR_.camera_.transform.position.x, -50.0f, 50.0f);
    ImGui::SliderFloat("Y", &vkR_.camera_.transform.position.y, -50.0f, 50.0f);
    ImGui::SliderFloat("Z", &vkR_.camera_.transform.position.z, -50.0f, 50.0f);
    ImGui::ColorEdit3("clear color", (float*)&vkR_.clearValue_.color);
    ImGui::DragFloat("cascadeLambda", &vkR_.pDirectionalLight_->cascadeSplitLambda, 0.01f);
    ImGui::DragFloat("bias", &vkR_.depthBias_);
    ImGui::DragFloat("reflectionLOD", &vkR_.maxReflectionLOD_);
    ImGui::Checkbox("rotate", &vkR_.rotate_);
    ImGui::End();
}

void GraphicsManager::updateModelMatrices() {
    int modelMatrixID = 0;
    for (auto& gameObject : staticGameObjects) {
        for (int i = 0; i < gameObject->renderTargets.size(); i++) {
            for (auto& mat : gameObject->renderTargets[i]->opaqueDraws) {
                for (auto& dC : mat.second) {
                    vkR_.modelMatrices[modelMatrixID] = gameObject->renderTargetTransforms[i]->matrix * dC->worldTransformMatrix;
                    modelMatrixID++;
                }
            }
        }
    }
    for (auto& gameObject : staticGameObjects) {
        for (int i = 0; i < gameObject->renderTargets.size(); i++) {
            for (auto& mat : gameObject->renderTargets[i]->transparentDraws) {
                for (auto& dC : mat.second) {
                    vkR_.modelMatrices[modelMatrixID] = gameObject->renderTargetTransforms[i]->matrix * dC->worldTransformMatrix;
                    modelMatrixID++;
                }
            }
        }
    }
    for (auto& animGameObject : animatedGameObjects) {
        for (const auto& renderTarget : animGameObject->renderTargets) {
            for (auto& mat : renderTarget->opaqueDraws) {
                for (auto& dC : mat.second) {
                    vkR_.modelMatrices[modelMatrixID] = animGameObject->transform.matrix * dC->worldTransformMatrix;
                    modelMatrixID++;
                }
            }
            for (auto& mat : renderTarget->transparentDraws) {
                for (auto& dC : mat.second) {
                    vkR_.modelMatrices[modelMatrixID] = animGameObject->transform.matrix * dC->worldTransformMatrix;
                    modelMatrixID++;
                }
            }
        }
    }
}

void GraphicsManager::startVulkan(std::vector<std::string>& staticModelPaths, std::vector<std::string>& animatedModelPaths, std::string& skyboxModelPath, std::vector<std::string>& skyboxTexturePaths) {
    vkR_.pDevHelper_ = new DeviceHelper();
    std::cout << "created: device helper" << std::endl;

    vkR_.camera_.update();

    vkR_.instance_ = vkR_.createVulkanInstance(pWindow_, "Vulkan Game Engine");
    std::cout << "created: vulkan instance" << std::endl;

    vkR_.setupDebugMessenger(vkR_.instance_, vkR_.debugMessenger_);

    vkR_.createSurface(pWindow_);
    std::cout << "created surface" << std::endl;

    vkR_.pickPhysicalDevice();
    vkR_.pDevHelper_->gpu_ = vkR_.GPU_;
    std::cout << "chose physical device" << std::endl;

    vkR_.createLogicalDevice();
    vkR_.pDevHelper_->device_ = vkR_.device_;
    vkR_.pDevHelper_->graphicsQueue_ = vkR_.graphicsQueue_;
    std::cout << "created logical device" << std::endl;

    vkR_.createSWChain(pWindow_);
    std::cout << "chreated swap chain" << std::endl;

    std::cout << "FRAMES IN FLIGHT: " << vkR_.numFramesInFlight << std::endl;

    vkR_.createImageViews();
    std::cout << "created swap chain image views" << std::endl;

    vkR_.createRenderPass();
    std::cout << "created render pass" << std::endl;

    vkR_.createCommandPool();
    vkR_.pDevHelper_->commandPool_ = vkR_.commandPool_;
    std::cout << "created command pool" << std::endl;

    vkR_.createColorResources();
    std::cout << "created color resources" << std::endl;

    vkR_.createDepthResources();
    std::cout << "created depth resources" << std::endl;

    vkR_.createFrameBuffer();
    std::cout << "created frame buffers" << std::endl;

    vkR_.createDescriptorPool();
    vkR_.pDevHelper_->descPool_ = vkR_.descriptorPool_;
    std::cout << "created descriptor pool" << std::endl;

    vkR_.pDirectionalLight_->setup(vkR_.pDevHelper_, &(vkR_.graphicsQueue_), &(vkR_.commandPool_), vkR_.SWChainExtent_.width, vkR_.SWChainExtent_.height);

    vkR_.camera_.setProjectionMatrix();
    vkR_.pDirectionalLight_->genShadowMap(&(vkR_.camera_), &(vkR_.modelMatrixSetLayout_->layout), vkR_.numFramesInFlight);

    std::cout << std::endl << "generated Shadow Map" << std::endl;

    vkR_.createUniformBuffers();
    std::cout << "created uniform buffers" << std::endl;

    vkR_.createDescriptorSetLayout();
    std::cout << "created desc set layout" << std::endl;

    vkR_.pDirectionalLight_->createPipeline(vkR_.modelMatrixSetLayout_);

    std::cout << "loading skybox\n" << std::endl;

    uint32_t globalVertexOffset = 6;
    uint32_t globalIndexOffset = 6;

    vkR_.vertices_ = { Vertex(glm::vec2(-1.0f, -1.0f), glm::vec2(0.0f, 0.0f)),
                       Vertex(glm::vec2(-1.0f, 1.0f), glm::vec2(0.0f, 1.0f)),
                       Vertex(glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 1.0f)),
                       Vertex(glm::vec2(-1.0f, -1.0f), glm::vec2(0.0f, 0.0f)),
                       Vertex(glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 1.0f)),
                       Vertex(glm::vec2(1.0f, -1.0f), glm::vec2(1.0f, 0.0f))
    };

    vkR_.indices_ = { 0, 1, 2, 3, 4, 5 };

    vkR_.pSkyBox_ = new Skybox(skyboxModelPath, skyboxTexturePaths, vkR_.pDevHelper_, globalVertexOffset, globalIndexOffset);
    vkR_.createSkyBoxPipeline();

    for (int i = 0; i < vkR_.pSkyBox_->pSkyBoxModel_->totalVertices_; i++) {
        vkR_.vertices_.push_back(vkR_.pSkyBox_->pSkyBoxModel_->vertices_[i]);
        globalVertexOffset++;
    }

    for (int i = 0; i < vkR_.pSkyBox_->pSkyBoxModel_->totalIndices_; i++) {
        vkR_.indices_.push_back(vkR_.pSkyBox_->pSkyBoxModel_->indices_[i]);
        globalIndexOffset++;
    }

    std::cout << "DONE loading skybox\n" << std::endl;

    vkR_.staticRenderTargets.resize(staticModelPaths.size());
    vkR_.animatedRenderTargets.resize(animatedModelPaths.size());

    for (int i = 0; i < staticModelPaths.size(); i++) {
        std::string s = staticModelPaths[i];
        vkR_.staticRenderTargets[i].setup(s, vkR_.pDevHelper_, globalVertexOffset, globalIndexOffset);

        vkR_.vertices_.insert(vkR_.vertices_.end(), vkR_.staticRenderTargets[i].vertices_.begin(), vkR_.staticRenderTargets[i].vertices_.end());
        vkR_.indices_.insert(vkR_.indices_.end(), vkR_.staticRenderTargets[i].indices_.begin(), vkR_.staticRenderTargets[i].indices_.end());

        vkR_.staticRenderTargets[i].indices_.clear();
        vkR_.staticRenderTargets[i].indices_.shrink_to_fit();
        vkR_.staticRenderTargets[i].vertices_.clear();
        vkR_.staticRenderTargets[i].vertices_.shrink_to_fit();

        globalVertexOffset = vkR_.vertices_.size();
        globalIndexOffset = vkR_.indices_.size();

        std::cout << "\nloaded model: " << s << ": " << vkR_.staticRenderTargets[i].totalVertices_ << " vertices, " << vkR_.staticRenderTargets[i].totalIndices_ << " indices\n" << std::endl;
    }

    uint32_t globalSkinMatrixOffset = 0;

    for (int i = 0; i < animatedModelPaths.size(); i++) {
        std::string s = animatedModelPaths[i];
        vkR_.animatedRenderTargets[i].setup(s, vkR_.pDevHelper_, globalVertexOffset, globalIndexOffset);

        vkR_.vertices_.insert(vkR_.vertices_.end(), vkR_.animatedRenderTargets[i].basePoseVertices_.begin(), vkR_.animatedRenderTargets[i].basePoseVertices_.end());
        vkR_.indices_.insert(vkR_.indices_.end(), vkR_.animatedRenderTargets[i].indices_.begin(), vkR_.animatedRenderTargets[i].indices_.end());

        vkR_.animatedRenderTargets[i].globalSkinningMatrixOffset = globalSkinMatrixOffset;

        for (auto& skin : vkR_.animatedRenderTargets[i].skins_) {
            for (glm::mat4& matrix : *(skin.finalJointMatrices)) {
                vkR_.inverseBindMatrices.push_back(matrix);
                globalSkinMatrixOffset++;
                vkR_.animatedRenderTargets[i].numInverseBindMatrices++;
            }
        }

        globalVertexOffset = vkR_.vertices_.size();
        globalIndexOffset = vkR_.indices_.size();

        MeshHelper::createVertexBuffer(vkR_.pDevHelper_, vkR_.animatedRenderTargets[i].basePoseVertices_, vkR_.animatedRenderTargets[i].vertexBuffer_, vkR_.animatedRenderTargets[i].vertexBufferMemory_);

        vkR_.animatedRenderTargets[i].indices_.clear();
        vkR_.animatedRenderTargets[i].indices_.shrink_to_fit();
        vkR_.animatedRenderTargets[i].basePoseVertices_.clear();
        vkR_.animatedRenderTargets[i].basePoseVertices_.shrink_to_fit();

        std::cout << "\nloaded model: " << s << ": " << vkR_.animatedRenderTargets[i].totalVertices_ << " vertices, " << vkR_.animatedRenderTargets[i].totalIndices_ << " indices\n" << std::endl;
    }

    vkR_.createVertexBuffer();
    vkR_.createIndexBuffer();

    vkR_.pDevHelper_->texDescSetLayout_ = vkR_.textureDescriptorSetLayout_->layout;

    vkR_.createDescriptorSets();
    std::cout << "created desc sets" << std::endl << std::endl;

    vkR_.brdfLut = new BRDFLut(vkR_.pDevHelper_);
    std::cout << "generated BRDFLUT" << std::endl;

    vkR_.irCube = new IrradianceCube(vkR_.pDevHelper_, vkR_.pSkyBox_, vkR_.vertexBuffer_, vkR_.indexBuffer_);
    std::cout << std::endl << "generated IrradianceCube" << std::endl;

    vkR_.prefEMap = new PrefilteredEnvMap(vkR_.pDevHelper_, vkR_.pSkyBox_, vkR_.vertexBuffer_, vkR_.indexBuffer_);

    std::cout << std::endl << "generated Prefiltered Environment Map" << std::endl;

    for (GLTFObj& gO : vkR_.staticRenderTargets) {
        gO.createDescriptors();
    }

    for (AnimatedGLTFObj& gO : vkR_.animatedRenderTargets) {
        gO.createDescriptors();
    }

    vkR_.updateGeneratedImageDescriptorSets();
    std::cout << "\ncreated descriptor sets" << std::endl;

    vkR_.createGraphicsPipeline();
    std::cout << "created material graphics pipeline" << std::endl;

    vkR_.createDepthPipeline();
    std::cout << "created depth pipeline" << std::endl;

    vkR_.createAlphaDepthPipeline();
    std::cout << "created depth alpha pipeline" << std::endl;

    vkR_.createToonPipeline();
    std::cout << "created cartoon graphics pipeline" << std::endl;

    vkR_.createOutlinePipeline();
    std::cout << "created outline pipeline" << std::endl;

    vkR_.createToneMappingPipeline();
    std::cout << "created tonemapping pipeline" << std::endl;

    vkR_.createCommandBuffers(vkR_.numFramesInFlight);
    std::cout << "created commaned buffers" << std::endl;

    vkR_.createSemaphores(vkR_.numFramesInFlight);
    std::cout << "created semaphores \n" << std::endl;

    vkR_.separateDrawCalls();
    std::cout << "separated draw calls \n" << std::endl;

    vkR_.setupCompute(vkR_.numFramesInFlight);
    std::cout << "setup compute \n" << std::endl;

    vkR_.bloomHelper = new BloomHelper(vkR_.pDevHelper_);
        
    vkR_.bloomHelper->setupBloom(&(vkR_.bloomResolveImage_), &(vkR_.bloomResolveImageView_), VK_FORMAT_R8G8B8A8_SRGB, vkR_.SWChainExtent_);
    std::cout << "setup bloom" << std::endl;

    return;
}

void GraphicsManager::loopUpdate() {
    imGUIUpdate();

    updateModelMatrices();
    vkR_.drawNewFrame(pWindow_, vkR_.numFramesInFlight);

    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), vkR_.commandBuffers_[vkR_.currentFrame_]);

    vkR_.postDrawEndCommandBuffer(vkR_.commandBuffers_[vkR_.currentFrame_], pWindow_, vkR_.numFramesInFlight);

    frameCount++;
}

GraphicsManager::GraphicsManager(float windowWidth, float windowHeight) {
    this->pRenderer_ = nullptr;
    this->pWindow_ = nullptr;
    this->windowWidth = windowWidth;
    this->windowHeight = windowHeight;
    this->frameCount = 0;
}