#include "VulkanRenderer.h"

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

VulkanRenderer::VulkanRenderer() {}

void VulkanRenderer::updateUniformBuffer(uint32_t currentImage) {
    this->camera_.setProjectionMatrix();

    UniformBufferObject ubo{
        .view = camera_.getViewMatrix(),
        .proj = camera_.getProjectionMatrix(),
        .lightPos = glm::vec4(pDirectionalLight_->transform.position, maxReflectionLOD_),
        .viewPos = glm::vec4(camera_.transform.position, depthBias_),
    };

    pDirectionalLight_->updateUniBuffers(&(this->camera_));
    for (int i = 0; i < SHADOW_MAP_CASCADE_COUNT; i++) {
        ubo.cascadeSplits[i] = pDirectionalLight_->cascades[i].splitDepth;
        ubo.cascadeViewProjMat[i] = pDirectionalLight_->cascades[i].viewProjectionMatrix;
    }

    ubo.gammaExposure.x = gamma_;
    ubo.gammaExposure.y = exposure_;
    ubo.gammaExposure.z = specularCont;

    ubo.gammaExposure.w = nDotVSpec;

    memcpy(mappedBuffers_[currentFrame_], &ubo, sizeof(UniformBufferObject));
}

void VulkanRenderer::drawNewFrame(SDL_Window * window, int maxFramesInFlight) {
    vkWaitForFences(this->device_, 1, &inFlightFences_[currentFrame_], VK_TRUE, UINT64_MAX);

    VkResult result = vkAcquireNextImageKHR(this->device_, this->swapChain_, UINT64_MAX, this->imageAcquiredSema_[currentFrame_], VK_NULL_HANDLE, &imageIndex_);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        this->recreateSwapChain(window);
        return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        std::cout << "no swap chain image, ya bum!" << std::endl;
        std::_Xruntime_error("Failed to acquire a swap chain image!");
    }

    updateUniformBuffer(imageIndex_);

    vkResetFences(this->device_, 1, &inFlightFences_[currentFrame_]);

    vkResetCommandBuffer(commandBuffers_[currentFrame_], 0);
    recordCommandBuffer(commandBuffers_[currentFrame_], imageIndex_);
}


void VulkanRenderer::renderBloom(VkCommandBuffer& commandBuffer) {
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &screenQuadVertexBuffer, offsets);
    vkCmdBindIndexBuffer(commandBuffer, screenQuadIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

    // down -------------------------------------------------------------------------------------------------------------
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bloomHelper->bloomPipelineDown);
    
    int mipLevel = 0;
    for (int i = 0; i < NUMBER_OF_DOWNSAMPLED_IMAGES; ++i) {
        bloomHelper->startBloomRenderPass(commandBuffer, &(bloomHelper->bloomRenderPassDown), &(bloomHelper->frameBuffersDown[i]));
        VkExtent2D extent{ SWChainExtent_.width >> (mipLevel + 1), SWChainExtent_.height >> (mipLevel + 1) };
        bloomHelper->setViewport(commandBuffer, extent);
    
        BloomHelper::pushConstantBloom push{};
    
        push.resolutionRadius = glm::vec4(extent.width, extent.height, this->bloomRadius, 0.0f);
    
        vkCmdPushConstants(commandBuffer, bloomHelper->bloomPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BloomHelper::pushConstantBloom), &push);
    
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bloomHelper->bloomPipelineLayout, 0, 1, &(bloomHelper->bloomSets[mipLevel]), 0, nullptr);
    
        vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffer);
        ++mipLevel;
    }

    // up -------------------------------------------------------------------------------------------------------------
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bloomHelper->bloomPipelineUp);

    mipLevel = NUMBER_OF_DOWNSAMPLED_IMAGES;
    for (int i = 0; i < NUMBER_OF_DOWNSAMPLED_IMAGES; ++i) {
        bloomHelper->startBloomRenderPass(commandBuffer, &(bloomHelper->bloomRenderPassUp), &(bloomHelper->frameBuffersUp[i]));
        VkExtent2D extent{ SWChainExtent_.width >> (mipLevel - 1), SWChainExtent_.height >> (mipLevel - 1) };
        bloomHelper->setViewport(commandBuffer, extent);

        BloomHelper::pushConstantBloom push{};

        push.resolutionRadius = glm::vec4(extent.width, extent.height, this->bloomRadius, 0.0f);

        vkCmdPushConstants(commandBuffer, bloomHelper->bloomPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(BloomHelper::pushConstantBloom), &push);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bloomHelper->bloomPipelineLayout, 0, 1, &(bloomHelper->bloomSets[mipLevel]), 0, nullptr);

        vkCmdDrawIndexed(commandBuffer, 6, 1, 0, 0, 0);

        vkCmdEndRenderPass(commandBuffer);
        --mipLevel;
    }
}

void VulkanRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo CBBeginInfo{};
    CBBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    CBBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffer, &CBBeginInfo) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to start recording with the command buffer!");
    }

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &computeDescriptorSet_, 0, nullptr);

    for (AnimatedGameObject* g : *animatedObjects) {
        const auto cs = ComputePushConstant{
            .jointMatrixStart = g->renderTarget->globalSkinningMatrixOffset,
            .numVertices = g->renderTarget->totalVertices_
        };
        vkCmdPushConstants(commandBuffer, computePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputePushConstant), &cs);

        static const auto workgroupSize = 256;
        const auto groupSizeX = (uint32_t)std::ceil(g->renderTarget->totalVertices_ / (float)workgroupSize);
        vkCmdDispatch(commandBuffer, groupSizeX, 1, 1);
    }

    VkMemoryBarrier2 memoryBarrier{};
    memoryBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
    memoryBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    memoryBarrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
    memoryBarrier.dstStageMask = VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT;
    memoryBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT;

    VkDependencyInfo dependencyInfo{};
    dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfo.memoryBarrierCount = 1;
    dependencyInfo.pMemoryBarriers = &memoryBarrier;

    vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);

    VkBuffer vertexBuffers[] = { vertexBuffer_ };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer_, 0, VK_INDEX_TYPE_UINT32);

    // DEPTH PREPASS //////////////////////////////////////////////////////////////////////////////////////////////
    VkClearValue clearValues[1];
    clearValues[0].depthStencil = { 1.0f, 0 };

    VkRenderPassBeginInfo depthPassBeginInfo{};
    depthPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    depthPassBeginInfo.renderPass = depthPrepass_;
    depthPassBeginInfo.renderArea.extent.width = SWChainExtent_.width;
    depthPassBeginInfo.renderArea.extent.height = SWChainExtent_.height;
    depthPassBeginInfo.clearValueCount = 1;
    depthPassBeginInfo.pClearValues = clearValues;
    depthPassBeginInfo.framebuffer = depthFrameBuffers_[currentFrame_];

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)SWChainExtent_.width;
    viewport.height = (float)SWChainExtent_.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = SWChainExtent_;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBeginRenderPass(commandBuffer, &depthPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, prepassPipeline_->pipeline);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, prepassPipeline_->layout, 0, 1, &descriptorSets_[this->currentFrame_], 0, nullptr);
    for (GameObject* gO : *(gameObjects)) {
        gO->renderTarget->renderDepth(commandBuffer, prepassPipeline_->layout);
    }
    for (AnimatedGameObject* animGO : *(animatedObjects)) {
        animGO->renderTarget->renderDepth(commandBuffer, prepassPipeline_->layout);
    }
    vkCmdEndRenderPass(commandBuffer);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pDirectionalLight_->sMPipeline_);

    for (uint32_t j = 0; j < SHADOW_MAP_CASCADE_COUNT; j++) {
        //VkDebugUtilsLabelEXT debugLabel{}; debugLabel.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT; debugLabel.pLabelName = std::string("Shadow Rendering, Cascade: " + j).c_str(); vkCmdBeginDebugUtilsLabelEXT(commandBuffer, &debugLabel);

        DirectionalLight::PostRenderPacket cmdBuf = pDirectionalLight_->render(commandBuffer, j);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pDirectionalLight_->sMPipelineLayout_, 0, 1, &(pDirectionalLight_->cascades[j].descriptorSet), 0, nullptr);
        for (GameObject* gO : *(gameObjects)) {
            gO->renderTarget->renderShadow(cmdBuf.commandBuffer, pDirectionalLight_->sMPipelineLayout_, j);
        }
        for (AnimatedGameObject* animGO : *(animatedObjects)) {
            animGO->renderTarget->renderShadow(cmdBuf.commandBuffer, pDirectionalLight_->sMPipelineLayout_, j);
        }
        vkCmdEndRenderPass(cmdBuf.commandBuffer);

        //vkCmdEndDebugUtilsLabelEXT(commandBuffer, &debugLabel);
    }

    // Start the scene render pass
    VkRenderPassBeginInfo RPBeginInfo{};
    // Create the render pass
    RPBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    RPBeginInfo.renderPass = renderPass_;
    RPBeginInfo.framebuffer = toneMappingFrameBuffers_[imageIndex];
    // Define the size of the render area
    RPBeginInfo.renderArea.offset = { 0, 0 };
    RPBeginInfo.renderArea.extent = SWChainExtent_;

    std::array<VkClearValue, 5> newClearValues{};
    newClearValues[0].color = clearValue_.color;
    newClearValues[1].color = clearValue_.color;
    newClearValues[3].color = clearValue_.color;
    newClearValues[4].color = clearValue_.color;

    // Define the clear values to use
    RPBeginInfo.clearValueCount = 5;
    RPBeginInfo.pClearValues = newClearValues.data();

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // Finally, begin the render pass
    vkCmdBeginRenderPass(commandBuffer, &RPBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    recordSkyBoxCommandBuffer(commandBuffer, imageIndex);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, opaquePipeline_->pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, opaquePipeline_->layout, 0, 1, &descriptorSets_[this->currentFrame_], 0, nullptr);

    for (GameObject* gO : *(gameObjects)) {
        vkCmdPushConstants(commandBuffer, opaquePipeline_->layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(float), &gO->isOutline);
        gO->renderTarget->drawIndexedOpaque(commandBuffer, opaquePipeline_->layout);
    }

    for (GameObject* gO : *(gameObjects)) {
        if (gO->renderTarget->transparentDraws.size() > 0) {
            vkCmdPushConstants(commandBuffer, opaquePipeline_->layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(float), &gO->isOutline);
            gO->renderTarget->drawIndexedTransparent(commandBuffer, opaquePipeline_->layout); // should be transparent
        }
    }

    for (AnimatedGameObject* gO : *(animatedObjects)) {
        if (gO->renderTarget->transparentDraws.size() > 0) {
            vkCmdPushConstants(commandBuffer, opaquePipeline_->layout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(float), &gO->isOutline);
            gO->renderTarget->drawIndexedTransparent(commandBuffer, opaquePipeline_->layout); // should be transparent
        }
    }

    // cartoon animated objects

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, toonPipeline_->pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, toonPipeline_->layout, 0, 1, &descriptorSets_[this->currentFrame_], 0, nullptr);

    for (AnimatedGameObject* gO : *(animatedObjects)) {
        gO->renderTarget->drawIndexedOpaque(commandBuffer, toonPipeline_->layout);
    }

    // outline last

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, outlinePipeline_->pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, outlinePipeline_->layout, 0, 1, &descriptorSets_[this->currentFrame_], 0, nullptr);

    for (AnimatedGameObject* gO : *(animatedObjects)) {
        if (gO->isOutline) {
            gO->renderTarget->drawIndexedOutline(commandBuffer, outlinePipeline_->layout);
        }
    }

    vkCmdEndRenderPass(commandBuffer);

    // render bloom

    renderBloom(commandBuffer);

    // tonemapping renderpass

    VkRenderPassBeginInfo tonemapRenderPassBI{};
    tonemapRenderPassBI.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    tonemapRenderPassBI.renderPass = toneMapPass_;
    tonemapRenderPassBI.framebuffer = SWChainFrameBuffers_[imageIndex];
    tonemapRenderPassBI.renderArea.offset = { 0, 0 };
    tonemapRenderPassBI.renderArea.extent = SWChainExtent_;

    tonemapRenderPassBI.clearValueCount = 1;
    tonemapRenderPassBI.pClearValues = clearValues;

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdBeginRenderPass(commandBuffer, &tonemapRenderPassBI, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, toneMappingPipeline_->pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, toneMappingPipeline_->layout, 0, 1, &descriptorSets_[this->currentFrame_], 0, nullptr);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, toneMappingPipeline_->layout, 1, 1, &toneMappingDescriptorSet_, 0, nullptr);

    vkCmdDrawIndexed(commandBuffer, 3, 1, 0, 0, 0);
    vkCmdDrawIndexed(commandBuffer, 3, 1, 3, 0, 0);
}

void VulkanRenderer::postDrawEndCommandBuffer(VkCommandBuffer commandBuffer, SDL_Window* window, int maxFramesInFlight) {
    // After drawing is over, end the render pass
    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        std::cout << "recording failed, you bum!" << std::endl;
        std::_Xruntime_error("Failed to record back the command buffer!");
    }

    // Submit the command buffer with the semaphore
    VkSubmitInfo queueSubmitInfo{};
    queueSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { this->imageAcquiredSema_[currentFrame_] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    queueSubmitInfo.waitSemaphoreCount = 1;
    queueSubmitInfo.pWaitSemaphores = waitSemaphores;
    queueSubmitInfo.pWaitDstStageMask = waitStages;

    // Specify the command buffers to actually submit for execution
    queueSubmitInfo.commandBufferCount = 1;
    queueSubmitInfo.pCommandBuffers = &this->commandBuffers_[currentFrame_];

    // Specify which semaphores to signal once command buffers have finished execution
    VkSemaphore signaledSemaphores[] = { this->renderedSema_[currentFrame_] };
    queueSubmitInfo.signalSemaphoreCount = 1;
    queueSubmitInfo.pSignalSemaphores = signaledSemaphores;

    // Finally, submit the queue info
    if (vkQueueSubmit(this->graphicsQueue_, 1, &queueSubmitInfo, inFlightFences_[currentFrame_]) != VK_SUCCESS) {
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
    VkSwapchainKHR swapChains[] = { this->swapChain_ };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex_;

    VkResult res2 = vkQueuePresentKHR(this->presentQueue_, &presentInfo);

    if (res2 == VK_ERROR_OUT_OF_DATE_KHR || res2 == VK_SUBOPTIMAL_KHR || this->frBuffResized_) {
        this->frBuffResized_ = false;
        this->recreateSwapChain(window);
    }
    else if (res2 != VK_SUCCESS) {
        std::_Xruntime_error("Failed to present a swap chain image!");
    }

    currentFrame_ = (currentFrame_ + 1) % maxFramesInFlight;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
CREATE SDL SURFACE
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderer::createSurface(SDL_Window* window) {
    SDL_Vulkan_CreateSurface(window, instance_, &surface_);
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

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
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
    aInfo.pEngineName = "Orchid";
    aInfo.engineVersion = 1;
    aInfo.apiVersion = VK_API_VERSION_1_3;

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
    if (vkCreateInstance(&instanceCInfo, nullptr, &instance_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }

    clearValue_.color = { {0.0f, 0.0f, 0.0f, 0.0f} };

    return instance_;
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
        if (availableFormat.format == VK_FORMAT_R8G8B8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
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
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface_, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface_, &formatCount, nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface_, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface_, &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface_, &presentModeCount, details.presentModes.data());
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

        if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            indices.computeFamily = i;
        }

        VkBool32 prSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface_, &prSupport);

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

void VulkanRenderer::createSWChain(SDL_Window* window) {
    SWChainSuppDetails swInfo = getDetails(GPU_);

    VkSurfaceFormatKHR surfaceFormat = swInfo.chooseSwSurfaceFormat(swInfo.formats);
    VkPresentModeKHR presentMode = swInfo.chooseSwPresMode(swInfo.presentModes);
    VkExtent2D extent = swInfo.chooseSwExtent(swInfo.capabilities, window);

    uint32_t numImages = swInfo.capabilities.minImageCount + 1;

    if (swInfo.capabilities.maxImageCount > 0 && numImages > swInfo.capabilities.maxImageCount) {
        numImages = swInfo.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = surface_;
    swapchainCreateInfo.minImageCount = numImages;
    swapchainCreateInfo.imageFormat = surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainCreateInfo.imageExtent = extent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

    QueueFamilyIndices indices = findQueueFamilies(GPU_);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if (indices.graphicsFamily != indices.presentFamily) {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    swapchainCreateInfo.preTransform = swInfo.capabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(device_, &swapchainCreateInfo, nullptr, &swapChain_) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    SWChainImageFormat_ = surfaceFormat.format;
    SWChainExtent_ = extent;
    SWChainImages_.resize(numImages);
    vkGetSwapchainImagesKHR(device_, swapChain_, &numImages, SWChainImages_.data());
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
    vkEnumeratePhysicalDevices(instance_, &numDevices, nullptr);
    if (numDevices == 0) {
        std::cout << "no GPUs found with Vulkan support!" << std::endl;
        std::_Xruntime_error("");
    }

    // Otherwise, store all handles in array
    std::vector<VkPhysicalDevice> devices(numDevices);
    vkEnumeratePhysicalDevices(instance_, &numDevices, devices.data());

    // Evaluate each, check if they are suitable
    for (const auto& device : devices) {
        if (isSuitable(device)) {
            GPU_ = device;
            pDevHelper_->msaaSamples_ = getMaxUsableSampleCount(GPU_);
            break;
        }
        else {
            std::cout << "not suitable!" << std::endl;
        }
    }

    if (GPU_ == VK_NULL_HANDLE) {
        std::cout << "could not find a suitable GPU!" << std::endl;
        std::_Xruntime_error("");
    }
}

void VulkanRenderer::createLogicalDevice() {
    QFIndices_ = findQueueFamilies(GPU_);

    std::vector<VkDeviceQueueCreateInfo> queuecInfos;
    std::set<uint32_t> uniqueQFamilies = { QFIndices_.graphicsFamily.value(), QFIndices_.presentFamily.value(), QFIndices_.computeFamily.value() };

    float queuePrio = 1.0f;
    for (uint32_t queueFamily : uniqueQFamilies) {
        VkDeviceQueueCreateInfo queuecInfo{};
        queuecInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queuecInfo.queueFamilyIndex = queueFamily;
        queuecInfo.queueCount = 1;
        queuecInfo.pQueuePriorities = &queuePrio;
        queuecInfos.push_back(queuecInfo);
    }

    VkPhysicalDeviceFeatures gpuFeatures{};
    gpuFeatures.samplerAnisotropy = VK_TRUE;
    gpuFeatures.depthClamp = VK_TRUE;
    gpuFeatures.imageCubeArray = VK_TRUE;

    VkPhysicalDeviceVulkan13Features vk13Features{};
    vk13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    vk13Features.synchronization2 = VK_TRUE;

    VkDeviceCreateInfo deviceCInfo{};
    deviceCInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCInfo.pNext = &vk13Features;
    deviceCInfo.queueCreateInfoCount = static_cast<uint32_t>(queuecInfos.size());
    deviceCInfo.pQueueCreateInfos = queuecInfos.data();
    deviceCInfo.pEnabledFeatures = &gpuFeatures;

    deviceCInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExts.size());
    deviceCInfo.ppEnabledExtensionNames = deviceExts.data();

    if (enableValLayers) {
        deviceCInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        deviceCInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else {
        deviceCInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(GPU_, &deviceCInfo, nullptr, &device_) != VK_SUCCESS) {
        std::_Xruntime_error("failed to instantiate logical device!");
    }

    vkGetDeviceQueue(device_, QFIndices_.graphicsFamily.value(), 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, QFIndices_.presentFamily.value(), 0, &presentQueue_);
    vkGetDeviceQueue(device_, QFIndices_.computeFamily.value(), 0, &computeQueue_);
}

void VulkanRenderer::loadDebugUtilsFunctions(VkDevice device) {
    vkCmdBeginDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddr(device, "vkCmdBeginDebugUtilsLabelEXT");
    if (!vkCmdBeginDebugUtilsLabelEXT) {
        std::cerr << "Failed to load vkCmdBeginDebugUtilsLabelEXT" << std::endl;
    }

    vkCmdEndDebugUtilsLabelEXT = (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetDeviceProcAddr(device, "vkCmdEndDebugUtilsLabelEXT");
    if (!vkCmdBeginDebugUtilsLabelEXT) {
        std::cerr << "Failed to load vkCmdBeginDebugUtilsLabelEXT" << std::endl;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
CREATING AND HANDLING IMAGE VIEWS
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderer::createImageViews() {
    SWChainImageViews_.resize(SWChainImages_.size());

    for (uint32_t i = 0; i < SWChainImages_.size(); i++) {
        pDevHelper_->createImageView(SWChainImages_[i], SWChainImageViews_[i], SWChainImageFormat_, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
CREATING THE RENDER PASS
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderer::createRenderPass() {
    VkAttachmentDescription colorAttachmentDescription{};
    colorAttachmentDescription.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    colorAttachmentDescription.samples = pDevHelper_->msaaSamples_;
    colorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription bloomAttachmentDescription{};
    bloomAttachmentDescription.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    bloomAttachmentDescription.samples = pDevHelper_->msaaSamples_;
    bloomAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    bloomAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    bloomAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    bloomAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    bloomAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    bloomAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachmentDescription{};
    depthAttachmentDescription.format = findDepthFormat();
    depthAttachmentDescription.samples = pDevHelper_->msaaSamples_;
    depthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    depthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachmentResolve{};
    colorAttachmentResolve.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    VkAttachmentDescription bloomResolveAttachment{};
    bloomResolveAttachment.format = VK_FORMAT_R16G16B16A16_SFLOAT;
    bloomResolveAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    bloomResolveAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    bloomResolveAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    bloomResolveAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    bloomResolveAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    bloomResolveAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    bloomResolveAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    // One render pass consists of multiple render subpasses, but we are only going to use 1 for the triangle
    VkAttachmentReference colorAttachmentReference{};
    colorAttachmentReference.attachment = 0;
    colorAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference bloomAttachmentReference{};
    bloomAttachmentReference.attachment = 1;
    bloomAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentReference{};
    depthAttachmentReference.attachment = 2;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentResolveRef{};
    colorAttachmentResolveRef.attachment = 3;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference bloomAttachmentResolveRef{};
    bloomAttachmentResolveRef.attachment = 4;
    bloomAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    std::vector<VkAttachmentReference> colorRefs = { colorAttachmentReference, bloomAttachmentReference };
    std::vector<VkAttachmentReference> resolveRefs = { colorAttachmentResolveRef, bloomAttachmentResolveRef };

    // Create the subpass
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 2;
    subpass.pColorAttachments = colorRefs.data();
    subpass.pDepthStencilAttachment = &depthAttachmentReference;
    subpass.pResolveAttachments = resolveRefs.data();

    // Create information struct for the render pass
    std::array<VkAttachmentDescription, 5> attachments = { colorAttachmentDescription, bloomAttachmentDescription, depthAttachmentDescription, colorAttachmentResolve, bloomResolveAttachment };

    const uint32_t numDependencies = 2;
    VkSubpassDependency subpassDependencies[numDependencies];

    VkSubpassDependency& depBegToColorBuffer = subpassDependencies[0];
    depBegToColorBuffer = {};
    depBegToColorBuffer.srcSubpass = VK_SUBPASS_EXTERNAL;
    depBegToColorBuffer.dstSubpass = 0;
    depBegToColorBuffer.srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    depBegToColorBuffer.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    depBegToColorBuffer.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    depBegToColorBuffer.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    depBegToColorBuffer.dependencyFlags = 0;

    VkSubpassDependency& depEndFromColorBuffer = subpassDependencies[1];
    depEndFromColorBuffer = {};
    depEndFromColorBuffer.srcSubpass = 0;
    depEndFromColorBuffer.dstSubpass = VK_SUBPASS_EXTERNAL;
    depEndFromColorBuffer.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    depEndFromColorBuffer.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    depEndFromColorBuffer.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    depEndFromColorBuffer.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    depEndFromColorBuffer.dependencyFlags = 0;

    VkRenderPassCreateInfo renderPassCInfo{};
    renderPassCInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassCInfo.pAttachments = attachments.data();
    renderPassCInfo.subpassCount = 1;
    renderPassCInfo.pSubpasses = &subpass;
    renderPassCInfo.dependencyCount = 2;
    renderPassCInfo.pDependencies = subpassDependencies;

    if (vkCreateRenderPass(device_, &renderPassCInfo, nullptr, &renderPass_) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create render pass!");
    }

    VkAttachmentDescription swAttachmentDescription{};
    swAttachmentDescription.format = this->SWChainImageFormat_;
    swAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    swAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    swAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    swAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    swAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    swAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    swAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference toneMapReference{};
    toneMapReference.attachment = 0;
    toneMapReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Create the subpass
    VkSubpassDescription toneSubpass{};
    toneSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    toneSubpass.colorAttachmentCount = 1;
    toneSubpass.pColorAttachments = &toneMapReference;

    std::array<VkSubpassDependency, 1> tondependencies;

    tondependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    tondependencies[0].dstSubpass = 0;
    tondependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    tondependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    tondependencies[0].srcAccessMask = 0;
    tondependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    tondependencies[0].dependencyFlags = 0;

    VkRenderPassCreateInfo toneMapRenderPassCInfo{};
    toneMapRenderPassCInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    toneMapRenderPassCInfo.attachmentCount = 1;
    toneMapRenderPassCInfo.pAttachments = &swAttachmentDescription;
    toneMapRenderPassCInfo.subpassCount = 1;
    toneMapRenderPassCInfo.pSubpasses = &toneSubpass;
    toneMapRenderPassCInfo.dependencyCount = 1;
    toneMapRenderPassCInfo.pDependencies = tondependencies.data();

    if (vkCreateRenderPass(device_, &toneMapRenderPassCInfo, nullptr, &toneMapPass_) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create render pass!");
    }

    VkAttachmentDescription depthZAttachmentDescription{};
    depthZAttachmentDescription.format = findDepthFormat();
    depthZAttachmentDescription.samples = pDevHelper_->msaaSamples_;
    depthZAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthZAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    depthZAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthZAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthZAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthZAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthZAttachmentReference{};
    depthZAttachmentReference.attachment = 0;
    depthZAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription zSubpass = {};
    zSubpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    zSubpass.pDepthStencilAttachment = &depthZAttachmentReference;

    std::array<VkSubpassDependency, 2> zDependencies;

    zDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    zDependencies[0].dstSubpass = 0;
    zDependencies[0].srcStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    zDependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    zDependencies[0].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    zDependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    zDependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    zDependencies[1].srcSubpass = 0;
    zDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    zDependencies[1].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    zDependencies[1].dstStageMask = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
    zDependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    zDependencies[1].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    zDependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo depthPrePassCInfo{};
    depthPrePassCInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    depthPrePassCInfo.attachmentCount = 1;
    depthPrePassCInfo.pAttachments = &depthZAttachmentDescription;
    depthPrePassCInfo.subpassCount = 1;
    depthPrePassCInfo.pSubpasses = &zSubpass;
    depthPrePassCInfo.dependencyCount = 2;
    depthPrePassCInfo.pDependencies = zDependencies.data();

    if (vkCreateRenderPass(device_, &depthPrePassCInfo, nullptr, &depthPrepass_) != VK_SUCCESS) {
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
    UBOLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
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

    VkDescriptorSetLayoutBinding samplerLayoutBindingBRDF{};
    samplerLayoutBindingBRDF.binding = 5;
    samplerLayoutBindingBRDF.descriptorCount = 1;
    samplerLayoutBindingBRDF.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBindingBRDF.pImmutableSamplers = nullptr;
    samplerLayoutBindingBRDF.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBindingIrradiance{};
    samplerLayoutBindingIrradiance.binding = 6;
    samplerLayoutBindingIrradiance.descriptorCount = 1;
    samplerLayoutBindingIrradiance.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBindingIrradiance.pImmutableSamplers = nullptr;
    samplerLayoutBindingIrradiance.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBindingPrefiltered{};
    samplerLayoutBindingPrefiltered.binding = 7;
    samplerLayoutBindingPrefiltered.descriptorCount = 1;
    samplerLayoutBindingPrefiltered.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBindingPrefiltered.pImmutableSamplers = nullptr;
    samplerLayoutBindingPrefiltered.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutShadow{};
    samplerLayoutShadow.binding = 8;
    samplerLayoutShadow.descriptorCount = 1;
    samplerLayoutShadow.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutShadow.pImmutableSamplers = nullptr;
    samplerLayoutShadow.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::vector<VkDescriptorSetLayoutBinding> samplerBindings = { samplerLayoutBindingColor, samplerLayoutBindingNormal, samplerLayoutBindingMetallicRoughness, samplerLayoutBindingAO, samplerLayoutBindingEmission, samplerLayoutBindingBRDF, samplerLayoutBindingIrradiance, samplerLayoutBindingPrefiltered, samplerLayoutShadow };

    VkDescriptorSetLayoutCreateInfo layoutCInfo{};
    layoutCInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCInfo.bindingCount = 1;
    layoutCInfo.pBindings = &UBOLayoutBinding;

    if (vkCreateDescriptorSetLayout(device_, &layoutCInfo, nullptr, &uniformDescriptorSetLayout_) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create the uniform descriptor set layout!");
    }

    layoutCInfo.bindingCount = 9;
    layoutCInfo.pBindings = samplerBindings.data();

    if (vkCreateDescriptorSetLayout(device_, &layoutCInfo, nullptr, &textureDescriptorSetLayout_) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create the texture descriptor set layout!");
    }

    VkDescriptorSetLayoutBinding tonemappingBinding{};
    tonemappingBinding.binding = 0;
    tonemappingBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    tonemappingBinding.descriptorCount = 1;
    tonemappingBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    tonemappingBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding tonemappingBloomBinding{};
    tonemappingBloomBinding.binding = 1;
    tonemappingBloomBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    tonemappingBloomBinding.descriptorCount = 1;
    tonemappingBloomBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    tonemappingBloomBinding.pImmutableSamplers = nullptr;

    std::array< VkDescriptorSetLayoutBinding, 2> tonemappingBindings = { tonemappingBinding, tonemappingBloomBinding };

    VkDescriptorSetLayoutCreateInfo tonemappingLayoutCInfo{};
    tonemappingLayoutCInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    tonemappingLayoutCInfo.bindingCount = 2;
    tonemappingLayoutCInfo.pBindings = tonemappingBindings.data();

    if (vkCreateDescriptorSetLayout(device_, &tonemappingLayoutCInfo, nullptr, &tonemappingDescriptorSetLayout_) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create the texture descriptor set layout!");
    }
}

void VulkanRenderer::createVertexBuffer() {
    VkDeviceSize bufferSize = sizeof(Vertex) * vertices_.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    pDevHelper_->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device_, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices_.data(), (size_t)bufferSize);
    vkUnmapMemory(device_, stagingBufferMemory);

    pDevHelper_->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer_, vertexBufferMemory_);
    pDevHelper_->copyBuffer(stagingBuffer, this->vertexBuffer_, bufferSize);

    createQuadVertexBuffer();
}

void VulkanRenderer::createIndexBuffer() {
    VkDeviceSize bufferSize = sizeof(indices_[0]) * indices_.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    pDevHelper_->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device_, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices_.data(), (size_t)bufferSize);
    vkUnmapMemory(device_, stagingBufferMemory);

    pDevHelper_->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer_, indexBufferMemory_);
    pDevHelper_->copyBuffer(stagingBuffer, indexBuffer_, bufferSize);

    createQuadIndexBuffer();
}

void VulkanRenderer::createQuadVertexBuffer() {
    screenQuadVertices = { Vertex(glm::vec2(-1.0f, -1.0f), glm::vec2(0.0f, 0.0f)),
                           Vertex(glm::vec2(-1.0f, 1.0f), glm::vec2(0.0f, 1.0f)),
                           Vertex(glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 1.0f)),
                           Vertex(glm::vec2(-1.0f, -1.0f), glm::vec2(0.0f, 0.0f)),
                           Vertex(glm::vec2(1.0f, 1.0f), glm::vec2(1.0f, 1.0f)),
                           Vertex(glm::vec2(1.0f, -1.0f), glm::vec2(1.0f, 0.0f))
    };

    VkDeviceSize bufferSize = sizeof(Vertex) * screenQuadVertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    pDevHelper_->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device_, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, screenQuadVertices.data(), (size_t)bufferSize);
    vkUnmapMemory(device_, stagingBufferMemory);

    pDevHelper_->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, screenQuadVertexBuffer, screenQuadVertexBufferMemory);
    pDevHelper_->copyBuffer(stagingBuffer, this->screenQuadVertexBuffer, bufferSize);
}

void VulkanRenderer::createQuadIndexBuffer() {
    for (int i = 0; i < 6; i++) {
        screenQuadIndices.push_back(i);
    }

    VkDeviceSize bufferSize = sizeof(screenQuadIndices[0]) * screenQuadIndices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    pDevHelper_->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device_, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, screenQuadIndices.data(), (size_t)bufferSize);
    vkUnmapMemory(device_, stagingBufferMemory);

    pDevHelper_->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, screenQuadIndexBuffer, screenQuadIndexBufferMemory);
    pDevHelper_->copyBuffer(stagingBuffer, screenQuadIndexBuffer, bufferSize);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
GRAPHICS PIPELINE
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderer::sortDraw(GLTFObj* obj, GLTFObj::SceneNode* node) {
    for (auto& mesh : node->meshPrimitives) {
        Material* mat = &(obj->mats_[mesh->materialIndex]);
        if (mat->alphaMode == "OPAQUE") {
            obj->opaqueDraws[mat].push_back(mesh);
        }
        else {
            obj->transparentDraws[mat].push_back(mesh);
        }
    }
    for (auto& child : node->children) {
        sortDraw(obj, child);
    }
}

void VulkanRenderer::sortDraw(AnimatedGLTFObj* animObj, AnimSceneNode* node) {
    for (auto& mesh : node->meshPrimitives) {
        Material* mat = &(animObj->mats_[mesh->materialIndex]);
        if (mat->alphaMode == "OPAQUE") {
            animObj->opaqueDraws[mat].push_back(mesh);
        }
        else {
            animObj->transparentDraws[mat].push_back(mesh);
        }
    }
    for (auto& child : node->children) {
        sortDraw(animObj, child);
    }
}

void VulkanRenderer::separateDrawCalls() {
    for (GameObject* g : *gameObjects) {
        GLTFObj* obj = g->renderTarget;
        for (auto& node : obj->pParentNodes) {
            sortDraw(obj, node);
        }
    }
    for (AnimatedGameObject* g : *animatedObjects) {
        AnimatedGLTFObj* obj = g->renderTarget;
        for (auto& node : obj->pParentNodes) {
            sortDraw(obj, node);
        }
    }
}

void VulkanRenderer::createDepthPipeline() {
    VulkanPipelineBuilder::VulkanShaderModule vertexShaderModule = VulkanPipelineBuilder::VulkanShaderModule(device_, "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/spv/depthPass.spv");
    VulkanPipelineBuilder::VulkanShaderModule fragmentShaderModule = VulkanPipelineBuilder::VulkanShaderModule(device_, "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/spv/depthPassAlpha.spv");

    std::array<VulkanPipelineBuilder::VulkanShaderModule, 2> shaderStages = { vertexShaderModule, fragmentShaderModule };

    VkPushConstantRange pcDepthRange{};
    pcDepthRange.offset = 0;
    pcDepthRange.size = sizeof(glm::mat4);
    pcDepthRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPushConstantRange fragRange{};
    fragRange.offset = sizeof(glm::mat4);
    fragRange.size = sizeof(int) + sizeof(float);
    fragRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array< VkPushConstantRange, 2> ranges = { pcDepthRange, fragRange };

    std::array<VkDescriptorSetLayout, 2> sets = { uniformDescriptorSetLayout_, textureDescriptorSetLayout_ };

    auto bindings = Vertex::getBindingDescription();
    auto attributes = Vertex::getDepthAttributeDescription();

    VulkanPipelineBuilder::PipelineBuilderInfo pipelineInfo{};
    pipelineInfo.pDescriptorSetLayouts = sets.data();
    pipelineInfo.numSets = sets.size();
    pipelineInfo.pShaderStages = shaderStages.data();
    pipelineInfo.numStages = shaderStages.size();
    pipelineInfo.pPushConstantRanges = ranges.data();
    pipelineInfo.numRanges = 2;
    pipelineInfo.vertexBindingDescriptions = &bindings;
    pipelineInfo.numVertexBindingDescriptions = 1;
    pipelineInfo.vertexAttributeDescriptions = attributes.data();
    pipelineInfo.numVertexAttributeDescriptions = static_cast<int>(attributes.size());

    prepassPipeline_ = new VulkanPipelineBuilder(device_, pipelineInfo, pDevHelper_);

    prepassPipeline_->info.pDepthStencilState->depthWriteEnable = VK_TRUE;
    prepassPipeline_->info.pDepthStencilState->depthCompareOp = VK_COMPARE_OP_LESS;

    prepassPipeline_->info.pColorBlendState->attachmentCount = 0;
    prepassPipeline_->info.pColorBlendState->pAttachments = nullptr;

    prepassPipeline_->generate(pipelineInfo, depthPrepass_);
}

void VulkanRenderer::createOutlinePipeline() {
    VulkanPipelineBuilder::VulkanShaderModule vertexShaderModule = VulkanPipelineBuilder::VulkanShaderModule(device_, "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/spv/outlineVert.spv");
    VulkanPipelineBuilder::VulkanShaderModule fragmentShaderModule = VulkanPipelineBuilder::VulkanShaderModule(device_, "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/spv/outlineFrag.spv");

    std::array<VulkanPipelineBuilder::VulkanShaderModule, 2> shaderStages = { vertexShaderModule, fragmentShaderModule };

    VkPushConstantRange pcRange{};
    pcRange.offset = 0;
    pcRange.size = sizeof(glm::mat4);
    pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    std::array< VkPushConstantRange, 1> ranges = { pcRange };

    std::array<VkDescriptorSetLayout, 2> sets = { uniformDescriptorSetLayout_, textureDescriptorSetLayout_ };

    auto bindings = Vertex::getBindingDescription();
    auto attributes = Vertex::getDepthAttributeDescription();

    VulkanPipelineBuilder::PipelineBuilderInfo pipelineInfo{};
    pipelineInfo.pDescriptorSetLayouts = sets.data();
    pipelineInfo.numSets = sets.size();
    pipelineInfo.pShaderStages = shaderStages.data();
    pipelineInfo.numStages = shaderStages.size();
    pipelineInfo.pPushConstantRanges = ranges.data();
    pipelineInfo.numRanges = 1;
    pipelineInfo.vertexBindingDescriptions = &bindings;
    pipelineInfo.numVertexBindingDescriptions = 1;
    pipelineInfo.vertexAttributeDescriptions = attributes.data();
    pipelineInfo.numVertexAttributeDescriptions = static_cast<int>(attributes.size());

    outlinePipeline_ = new VulkanPipelineBuilder(device_, pipelineInfo, pDevHelper_);

    outlinePipeline_->info.pRasterizationState->cullMode = VK_CULL_MODE_FRONT_BIT;
    outlinePipeline_->info.pDepthStencilState->depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;

    outlinePipeline_->generate(pipelineInfo, renderPass_);
}

void VulkanRenderer::createToonPipeline() {
    VulkanPipelineBuilder::VulkanShaderModule vertexShaderModule = VulkanPipelineBuilder::VulkanShaderModule(device_, "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/spv/vert.spv");
    VulkanPipelineBuilder::VulkanShaderModule fragmentShaderModule = VulkanPipelineBuilder::VulkanShaderModule(device_, "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/spv/toonFrag.spv");

    std::array<VulkanPipelineBuilder::VulkanShaderModule, 2> shaderStages = { vertexShaderModule, fragmentShaderModule };

    VkPushConstantRange pcRange{};
    pcRange.offset = 0;
    pcRange.size = sizeof(glm::mat4);
    pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPushConstantRange fragRange{};
    fragRange.offset = sizeof(glm::mat4);
    fragRange.size = sizeof(float);
    fragRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array< VkPushConstantRange, 2> ranges = { pcRange, fragRange };

    std::array<VkDescriptorSetLayout, 2> sets = { uniformDescriptorSetLayout_, textureDescriptorSetLayout_ };

    auto bindings = Vertex::getBindingDescription();
    auto attributes = Vertex::getAttributeDescriptions();

    VulkanPipelineBuilder::PipelineBuilderInfo pipelineInfo{};
    pipelineInfo.pDescriptorSetLayouts = sets.data();
    pipelineInfo.numSets = sets.size();
    pipelineInfo.pShaderStages = shaderStages.data();
    pipelineInfo.numStages = shaderStages.size();
    pipelineInfo.pPushConstantRanges = ranges.data();
    pipelineInfo.numRanges = 2;
    pipelineInfo.vertexBindingDescriptions = &bindings;
    pipelineInfo.numVertexBindingDescriptions = 1;
    pipelineInfo.vertexAttributeDescriptions = attributes.data();
    pipelineInfo.numVertexAttributeDescriptions = static_cast<int>(attributes.size());

    toonPipeline_ = new VulkanPipelineBuilder(device_, pipelineInfo, pDevHelper_);
    toonPipeline_->generate(pipelineInfo, renderPass_);
}

void VulkanRenderer::createToneMappingPipeline() {
    VulkanPipelineBuilder::VulkanShaderModule vertexShaderModule = VulkanPipelineBuilder::VulkanShaderModule(device_, "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/spv/screenQuadVert.spv");
    VulkanPipelineBuilder::VulkanShaderModule fragmentShaderModule = VulkanPipelineBuilder::VulkanShaderModule(device_, "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/spv/tonemappingFrag.spv");

    std::array<VulkanPipelineBuilder::VulkanShaderModule, 2> shaderStages = { vertexShaderModule, fragmentShaderModule };

    VkPushConstantRange pcRange{};
    pcRange.offset = 0;
    pcRange.size = sizeof(glm::mat4);
    pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPushConstantRange fragRange{};
    fragRange.offset = sizeof(glm::mat4);
    fragRange.size = sizeof(float);
    fragRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array< VkPushConstantRange, 2> ranges = { pcRange, fragRange };

    std::array<VkDescriptorSetLayout, 2> sets = { uniformDescriptorSetLayout_, tonemappingDescriptorSetLayout_ };

    auto bindings = Vertex::getBindingDescription();
    auto attributes = Vertex::getPositionAttributeDescription();

    VulkanPipelineBuilder::PipelineBuilderInfo pipelineInfo{};
    pipelineInfo.pDescriptorSetLayouts = sets.data();
    pipelineInfo.numSets = sets.size();
    pipelineInfo.pShaderStages = shaderStages.data();
    pipelineInfo.numStages = shaderStages.size();
    pipelineInfo.pPushConstantRanges = ranges.data();
    pipelineInfo.numRanges = 2;
    pipelineInfo.vertexBindingDescriptions = &bindings;
    pipelineInfo.numVertexBindingDescriptions = 1;
    pipelineInfo.vertexAttributeDescriptions = attributes.data();
    pipelineInfo.numVertexAttributeDescriptions = static_cast<int>(attributes.size());

    toneMappingPipeline_ = new VulkanPipelineBuilder(device_, pipelineInfo, pDevHelper_);

    toneMappingPipeline_->info.pDepthStencilState->depthTestEnable = VK_FALSE;
    toneMappingPipeline_->info.pMultisampleState->rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    toneMappingPipeline_->info.pColorBlendState->attachmentCount = 1;

    toneMappingPipeline_->generate(pipelineInfo, toneMapPass_);
}

void VulkanRenderer::createGraphicsPipeline() {
    VulkanPipelineBuilder::VulkanShaderModule vertexShaderModule = VulkanPipelineBuilder::VulkanShaderModule(device_, "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/spv/vert.spv");
    VulkanPipelineBuilder::VulkanShaderModule fragmentShaderModule = VulkanPipelineBuilder::VulkanShaderModule(device_, "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/spv/frag.spv");

    std::array<VulkanPipelineBuilder::VulkanShaderModule, 2> shaderStages = { vertexShaderModule, fragmentShaderModule };

    VkPushConstantRange pcRange{};
    pcRange.offset = 0;
    pcRange.size = sizeof(glm::mat4);
    pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkPushConstantRange fragRange{};
    fragRange.offset = sizeof(glm::mat4);
    fragRange.size = sizeof(float);
    fragRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array< VkPushConstantRange, 2> ranges = { pcRange, fragRange };

    std::array<VkDescriptorSetLayout, 2> sets = { uniformDescriptorSetLayout_, textureDescriptorSetLayout_ };

    auto bindings = Vertex::getBindingDescription();
    auto attributes = Vertex::getAttributeDescriptions();

    VulkanPipelineBuilder::PipelineBuilderInfo pipelineInfo{};
    pipelineInfo.pDescriptorSetLayouts = sets.data();
    pipelineInfo.numSets = sets.size();
    pipelineInfo.pShaderStages = shaderStages.data();
    pipelineInfo.numStages = shaderStages.size();
    pipelineInfo.pPushConstantRanges = ranges.data();
    pipelineInfo.numRanges = 2;
    pipelineInfo.vertexBindingDescriptions = &bindings;
    pipelineInfo.numVertexBindingDescriptions = 1;
    pipelineInfo.vertexAttributeDescriptions = attributes.data();
    pipelineInfo.numVertexAttributeDescriptions = static_cast<int>(attributes.size());

    opaquePipeline_ = new VulkanPipelineBuilder(device_, pipelineInfo, pDevHelper_);
    opaquePipeline_->generate(pipelineInfo, renderPass_);
}

void VulkanRenderer::createSkyBoxPipeline() {
    VulkanPipelineBuilder::VulkanShaderModule vertexShaderModule = VulkanPipelineBuilder::VulkanShaderModule(device_, "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/spv/skyboxVert.spv");
    VulkanPipelineBuilder::VulkanShaderModule fragmentShaderModule = VulkanPipelineBuilder::VulkanShaderModule(device_, "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/spv/skyboxFrag.spv");

    std::array<VulkanPipelineBuilder::VulkanShaderModule, 2> shaderStages = { vertexShaderModule, fragmentShaderModule };

    VkPushConstantRange pcRange{};
    pcRange.offset = 0;
    pcRange.size = sizeof(glm::mat4);
    pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    std::array< VkPushConstantRange, 1> ranges = { pcRange };

    std::array<VkDescriptorSetLayout, 2> sets = { uniformDescriptorSetLayout_, pSkyBox_->skyBoxDescriptorSetLayout_ };

    auto bindings = Vertex::getBindingDescription();
    auto attributes = Vertex::getPositionAttributeDescription();

    VulkanPipelineBuilder::PipelineBuilderInfo pipelineInfo{};
    pipelineInfo.pDescriptorSetLayouts = sets.data();
    pipelineInfo.numSets = sets.size();
    pipelineInfo.pShaderStages = shaderStages.data();
    pipelineInfo.numStages = shaderStages.size();
    pipelineInfo.pPushConstantRanges = ranges.data();
    pipelineInfo.numRanges = 1;
    pipelineInfo.vertexBindingDescriptions = &bindings;
    pipelineInfo.numVertexBindingDescriptions = 1;
    pipelineInfo.vertexAttributeDescriptions = attributes.data();
    pipelineInfo.numVertexAttributeDescriptions = static_cast<int>(attributes.size());

    pSkyBox_->skyBoxPipeline_ = new VulkanPipelineBuilder(device_, pipelineInfo, pDevHelper_);

    pSkyBox_->skyBoxPipeline_->info.pDepthStencilState->depthTestEnable = VK_FALSE;
    pSkyBox_->skyBoxPipeline_->info.pRasterizationState->cullMode = VK_CULL_MODE_FRONT_BIT;

    pSkyBox_->skyBoxPipeline_->generate(pipelineInfo, renderPass_);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
CREATING THE FRAME BUFFER
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderer::createFrameBuffer() {
    SWChainFrameBuffers_.resize(SWChainImageViews_.size());
    
    depthFrameBuffers_.resize(SWChainImageViews_.size());

    toneMappingFrameBuffers_.resize(SWChainImageViews_.size());

    // Iterate through the image views and create framebuffers from them
    for (size_t i = 0; i < SWChainImageViews_.size(); i++) {
        std::array<VkImageView, 5> attachmentsStandard = { colorImageView_, bloomImageView_, depthImageView_, resolveImageView_, bloomResolveImageView_};

        VkFramebufferCreateInfo frameBufferCInfo{};
        frameBufferCInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        frameBufferCInfo.renderPass = renderPass_;
        frameBufferCInfo.attachmentCount = static_cast<uint32_t>(attachmentsStandard.size());
        frameBufferCInfo.pAttachments = attachmentsStandard.data();
        frameBufferCInfo.width = SWChainExtent_.width;
        frameBufferCInfo.height = SWChainExtent_.height;
        frameBufferCInfo.layers = 1;

        if (vkCreateFramebuffer(device_, &frameBufferCInfo, nullptr, &toneMappingFrameBuffers_[i]) != VK_SUCCESS) {
            std::_Xruntime_error("Failed to create a framebuffer for an image view!");
        }

        VkFramebufferCreateInfo depthFrameBufferCInfo{};
        depthFrameBufferCInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        depthFrameBufferCInfo.renderPass = depthPrepass_;
        depthFrameBufferCInfo.attachmentCount = 1;
        depthFrameBufferCInfo.pAttachments = &depthImageView_;
        depthFrameBufferCInfo.width = SWChainExtent_.width;
        depthFrameBufferCInfo.height = SWChainExtent_.height;
        depthFrameBufferCInfo.layers = 1;

        if (vkCreateFramebuffer(device_, &depthFrameBufferCInfo, nullptr, &depthFrameBuffers_[i]) != VK_SUCCESS) {
            std::_Xruntime_error("Failed to create a framebuffer for an image view!");
        }

        VkFramebufferCreateInfo toneBufferCInfo{};
        toneBufferCInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        toneBufferCInfo.renderPass = toneMapPass_;
        toneBufferCInfo.attachmentCount = 1;
        toneBufferCInfo.pAttachments = &(SWChainImageViews_[i]);
        toneBufferCInfo.width = SWChainExtent_.width;
        toneBufferCInfo.height = SWChainExtent_.height;
        toneBufferCInfo.layers = 1;

        if (vkCreateFramebuffer(device_, &toneBufferCInfo, nullptr, &SWChainFrameBuffers_[i]) != VK_SUCCESS) {
            std::_Xruntime_error("Failed to create a framebuffer for an image view!");
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
CREATE THE VERTEX, INDEX, AND UNIFORM BUFFERS AND OTHER HELPER METHODS
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderer::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers_.resize(SWChainImages_.size());
    uniformBuffersMemory_.resize(SWChainImages_.size());
    mappedBuffers_.resize(SWChainImages_.size());

    for (size_t i = 0; i < SWChainImages_.size(); i++) {
        pDevHelper_->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers_[i], uniformBuffersMemory_[i]);
        vkMapMemory(this->device_, this->uniformBuffersMemory_[i], 0, VK_WHOLE_SIZE, 0, &mappedBuffers_[i]);
        updateUniformBuffer(i);
    }
}

void VulkanRenderer::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 3> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = 1000; //static_cast<uint32_t>(SWChainImages_.size());
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    // Multiplied by 2 for imgui, needs to render separate font atlas, so needs double the image space // 5 samplers + 3 generated images + 1 shadow map
    poolSizes[1].descriptorCount = 1000; //(this->numMats_) * 10 + static_cast<uint32_t>(SWChainImages_.size()) + 6; // plus one for the skybox descriptor
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[2].descriptorCount = 3;

    VkDescriptorPoolCreateInfo poolCInfo{};
    poolCInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolCInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolCInfo.pPoolSizes = poolSizes.data();
    //  needs to render separate font atlas ( + 1)
    poolCInfo.maxSets = 1000; //(this->numMats_ * this->numImages_) + static_cast<uint32_t>(SWChainImages_.size()) + 1;

    if (vkCreateDescriptorPool(device_, &poolCInfo, nullptr, &descriptorPool_) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create the descriptor pool!");
    }
}

void VulkanRenderer::createDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(SWChainImages_.size(), uniformDescriptorSetLayout_);
    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = descriptorPool_;
    allocateInfo.descriptorSetCount = static_cast<uint32_t>(SWChainImages_.size());
    allocateInfo.pSetLayouts = layouts.data();

    descriptorSets_.resize(SWChainImages_.size());
    VkResult res = vkAllocateDescriptorSets(device_, &allocateInfo, descriptorSets_.data());
    if (res != VK_SUCCESS) {
        std::cout << res << std::endl;
        std::_Xruntime_error("Failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < SWChainImages_.size(); i++) {
        VkDescriptorBufferInfo descriptorBufferInfo{};
        descriptorBufferInfo.buffer = uniformBuffers_[i];
        descriptorBufferInfo.offset = 0;
        descriptorBufferInfo.range = sizeof(UniformBufferObject);

        VkWriteDescriptorSet descriptorWriteSet{};
        descriptorWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWriteSet.dstSet = descriptorSets_[i];
        descriptorWriteSet.dstBinding = 0;
        descriptorWriteSet.dstArrayElement = 0;
        descriptorWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWriteSet.descriptorCount = 1;
        descriptorWriteSet.pBufferInfo = &descriptorBufferInfo;

        vkUpdateDescriptorSets(device_, 1, &descriptorWriteSet, 0, nullptr);
    }

    VkDescriptorSetAllocateInfo toneMappingAllocateInfo{};
    toneMappingAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    toneMappingAllocateInfo.descriptorPool = descriptorPool_;
    toneMappingAllocateInfo.descriptorSetCount = 1;
    toneMappingAllocateInfo.pSetLayouts = &tonemappingDescriptorSetLayout_;

    VkResult res2 = vkAllocateDescriptorSets(device_, &toneMappingAllocateInfo, &toneMappingDescriptorSet_);
    if (res2 != VK_SUCCESS) {
        std::cout << res2 << std::endl;
        std::_Xruntime_error("Failed to allocate descriptor sets!");
    }

    VkSamplerCreateInfo samplerCInfo{};
    samplerCInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCInfo.magFilter = VK_FILTER_LINEAR;
    samplerCInfo.minFilter = VK_FILTER_LINEAR;
    samplerCInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerCInfo.mipLodBias = 0.0f;
    samplerCInfo.compareOp = VK_COMPARE_OP_NEVER;
    samplerCInfo.minLod = 0.0f;
    samplerCInfo.maxLod = 1.0f;
    samplerCInfo.anisotropyEnable = VK_TRUE;

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(pDevHelper_->gpu_, &properties);

    samplerCInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerCInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerCInfo.mipLodBias = 0.0f;

    if (vkCreateSampler(pDevHelper_->device_, &samplerCInfo, nullptr, &toneMappingSampler_) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create the texture sampler!");
    }

    VkSamplerCreateInfo bloomSamplerCInfo{};
    bloomSamplerCInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    bloomSamplerCInfo.magFilter = VK_FILTER_LINEAR;
    bloomSamplerCInfo.minFilter = VK_FILTER_LINEAR;
    bloomSamplerCInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    bloomSamplerCInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    bloomSamplerCInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    bloomSamplerCInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    bloomSamplerCInfo.mipLodBias = 0.0f;
    bloomSamplerCInfo.compareOp = VK_COMPARE_OP_NEVER;
    bloomSamplerCInfo.minLod = 0.0f;
    bloomSamplerCInfo.maxLod = 1.0f;
    bloomSamplerCInfo.anisotropyEnable = VK_TRUE;
    bloomSamplerCInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    bloomSamplerCInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    bloomSamplerCInfo.mipLodBias = 0.0f;

    if (vkCreateSampler(pDevHelper_->device_, &bloomSamplerCInfo, nullptr, &toneMappingBloomSampler_) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create the texture sampler!");
    }

    VkDescriptorImageInfo descriptorImageInfo{};
    descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descriptorImageInfo.imageView = resolveImageView_;
    descriptorImageInfo.sampler = toneMappingSampler_;

    VkDescriptorImageInfo descriptorImageBloomInfo{};
    descriptorImageBloomInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    descriptorImageBloomInfo.imageView = bloomResolveImageView_;
    descriptorImageBloomInfo.sampler = toneMappingBloomSampler_;

    VkWriteDescriptorSet descriptorWriteSet{};
    descriptorWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWriteSet.dstSet = toneMappingDescriptorSet_;
    descriptorWriteSet.dstBinding = 0;
    descriptorWriteSet.dstArrayElement = 0;
    descriptorWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWriteSet.descriptorCount = 1;
    descriptorWriteSet.pImageInfo = &descriptorImageInfo;

    VkWriteDescriptorSet bloomDescriptorWriteSet{};
    bloomDescriptorWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    bloomDescriptorWriteSet.dstSet = toneMappingDescriptorSet_;
    bloomDescriptorWriteSet.dstBinding = 1;
    bloomDescriptorWriteSet.dstArrayElement = 0;
    bloomDescriptorWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bloomDescriptorWriteSet.descriptorCount = 1;
    bloomDescriptorWriteSet.pImageInfo = &descriptorImageBloomInfo;

    std::array<VkWriteDescriptorSet, 2> descWrites = { descriptorWriteSet, bloomDescriptorWriteSet };

    vkUpdateDescriptorSets(device_, 2, descWrites.data(), 0, nullptr);
}

void VulkanRenderer::updateIndividualDescriptorSet(Material& m) {
    VkDescriptorImageInfo BRDFLutImageInfo{};
    BRDFLutImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    BRDFLutImageInfo.imageView = brdfLut->brdfLUTImageView_;
    BRDFLutImageInfo.sampler = brdfLut->brdfLUTImageSampler_;

    VkDescriptorImageInfo IrradianceImageInfo{};
    IrradianceImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    IrradianceImageInfo.imageView = irCube->iRCubeImageView_;
    IrradianceImageInfo.sampler = irCube->iRCubeImageSampler_;

    VkDescriptorImageInfo PrefilteredEnvMapInfo{};
    PrefilteredEnvMapInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    PrefilteredEnvMapInfo.imageView = prefEMap->prefEMapImageView_;
    PrefilteredEnvMapInfo.sampler = prefEMap->prefEMapImageSampler_;

    VkDescriptorImageInfo shadowMpaInfo{};
    shadowMpaInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    shadowMpaInfo.imageView = pDirectionalLight_->sMImageView_;
    shadowMpaInfo.sampler = pDirectionalLight_->sMImageSampler_;

    VkWriteDescriptorSet BRDFLutWrite{};
    BRDFLutWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    BRDFLutWrite.dstSet = m.descriptorSet;
    BRDFLutWrite.dstBinding = 5;
    BRDFLutWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    BRDFLutWrite.descriptorCount = 1;
    BRDFLutWrite.pImageInfo = &BRDFLutImageInfo;

    VkWriteDescriptorSet IrradianceWrite{};
    IrradianceWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    IrradianceWrite.dstSet = m.descriptorSet;
    IrradianceWrite.dstBinding = 6;
    IrradianceWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    IrradianceWrite.descriptorCount = 1;
    IrradianceWrite.pImageInfo = &IrradianceImageInfo;

    VkWriteDescriptorSet prefilteredWrite{};
    prefilteredWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    prefilteredWrite.dstSet = m.descriptorSet;
    prefilteredWrite.dstBinding = 7;
    prefilteredWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    prefilteredWrite.descriptorCount = 1;
    prefilteredWrite.pImageInfo = &PrefilteredEnvMapInfo;

    VkWriteDescriptorSet shadowWrite{};
    shadowWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    shadowWrite.dstSet = m.descriptorSet;
    shadowWrite.dstBinding = 8;
    shadowWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    shadowWrite.descriptorCount = 1;
    shadowWrite.pImageInfo = &shadowMpaInfo;

    std::vector<VkWriteDescriptorSet> descriptorWriteSets;
    descriptorWriteSets.push_back(BRDFLutWrite);
    descriptorWriteSets.push_back(IrradianceWrite);
    descriptorWriteSets.push_back(prefilteredWrite);
    descriptorWriteSets.push_back(shadowWrite);

    vkUpdateDescriptorSets(pDevHelper_->device_, static_cast<uint32_t>(descriptorWriteSets.size()), descriptorWriteSets.data(), 0, nullptr);
}

void VulkanRenderer::updateGeneratedImageDescriptorSets() {
    for (GameObject* gO : *gameObjects) {
        for (Material& m : gO->renderTarget->mats_) {
            updateIndividualDescriptorSet(m);
        }
    }
    for (AnimatedGameObject* aGO : *animatedObjects) {
        for (Material& m : aGO->renderTarget->mats_) {
            updateIndividualDescriptorSet(m);
        }
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
        vkGetPhysicalDeviceFormatProperties(GPU_, format, &props);

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
    pDevHelper_->createImage(SWChainExtent_.width, SWChainExtent_.height, 1, 1, static_cast<VkImageCreateFlagBits>(0), pDevHelper_->msaaSamples_, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage_, colorImageMemory_);
    pDevHelper_->createImageView(colorImage_, colorImageView_, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    pDevHelper_->createImage(SWChainExtent_.width, SWChainExtent_.height, 1, 1, static_cast<VkImageCreateFlagBits>(0), pDevHelper_->msaaSamples_, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, bloomImage_, bloomImageMemory_);
    pDevHelper_->createImageView(bloomImage_, bloomImageView_, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    pDevHelper_->createImage(SWChainExtent_.width, SWChainExtent_.height, 1, 1, static_cast<VkImageCreateFlagBits>(0), VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, resolveImage_, resolveImageMemory_);
    pDevHelper_->createImageView(resolveImage_, resolveImageView_, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    pDevHelper_->createImage(SWChainExtent_.width, SWChainExtent_.height, BLOOM_LEVELS, 1, static_cast<VkImageCreateFlagBits>(0), VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, bloomResolveImage_, bloomResolveImageMemory_);
    pDevHelper_->createImageView(bloomResolveImage_, bloomResolveImageView_, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

void VulkanRenderer::createDepthResources() {
    VkFormat depthFormat = findDepthFormat();
    pDevHelper_->createImage(SWChainExtent_.width, SWChainExtent_.height, 1, 1, static_cast<VkImageCreateFlagBits>(0), pDevHelper_->msaaSamples_, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage_, depthImageMemory_);
    pDevHelper_->createImageView(depthImage_, depthImageView_, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
CREATING THE COMMAND POOL AND BUFFER
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderer::createCommandPool() {
    QFIndices_ = findQueueFamilies(GPU_);

    // Creating the command pool create information struct
    VkCommandPoolCreateInfo commandPoolCInfo{};
    commandPoolCInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolCInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandPoolCInfo.queueFamilyIndex = QFIndices_.graphicsFamily.value();

    // Actual creation of the command buffer
    if (vkCreateCommandPool(device_, &commandPoolCInfo, nullptr, &commandPool_) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to create a command pool!");
    }
}

void VulkanRenderer::createCommandBuffers(int numFramesInFlight) {
    commandBuffers_.resize(numFramesInFlight);

    // Information to allocate the frame buffer
    VkCommandBufferAllocateInfo CBAllocateInfo{};
    CBAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    CBAllocateInfo.commandPool = commandPool_;
    CBAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    CBAllocateInfo.commandBufferCount = (uint32_t)commandBuffers_.size();

    if (vkAllocateCommandBuffers(device_, &CBAllocateInfo, commandBuffers_.data()) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to allocate a command buffer!");
    }
}

void VulkanRenderer::recordSkyBoxCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pSkyBox_->skyBoxPipeline_->pipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pSkyBox_->skyBoxPipeline_->layout, 0, 1, &descriptorSets_[this->currentFrame_], 0, nullptr);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pSkyBox_->skyBoxPipeline_->layout, 1, 1, &(pSkyBox_->skyBoxDescriptorSet_), 0, nullptr);
    pSkyBox_->drawSkyBoxIndexed(commandBuffer);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
INITIALIZING THE TWO SEMAPHORES AND THE FENCES
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderer::createSemaphores(const int maxFramesInFlight) {
    imageAcquiredSema_.resize(maxFramesInFlight);
    renderedSema_.resize(maxFramesInFlight);
    inFlightFences_.resize(maxFramesInFlight);

    VkSemaphoreCreateInfo semaCInfo{};
    semaCInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceCInfo{};
    fenceCInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < maxFramesInFlight; i++) {
        if (vkCreateSemaphore(device_, &semaCInfo, nullptr, &imageAcquiredSema_[i]) != VK_SUCCESS || vkCreateSemaphore(device_, &semaCInfo, nullptr, &renderedSema_[i]) != VK_SUCCESS || vkCreateFence(device_, &fenceCInfo, nullptr, &inFlightFences_[i]) != VK_SUCCESS) {
            std::cout << "bum" << std::endl;
            std::_Xruntime_error("Failed to create the synchronization objects for a frame!");
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
COMPUTE
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderer::updateBindMatrices() {
    memcpy(mappedSkinBuffer, inverseBindMatrices.data(), inverseBindMatrices.size() * sizeof(glm::mat4));
}

void VulkanRenderer::setupCompute() {
    size_t bufferSize = inverseBindMatrices.size() * sizeof(glm::mat4);

    pDevHelper_->createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, skinBindMatricsBuffer, skinBindMatricesBufferMemory);

    vkMapMemory(pDevHelper_->device_, skinBindMatricesBufferMemory, 0, bufferSize, 0, &mappedSkinBuffer);
    memcpy(mappedSkinBuffer, inverseBindMatrices.data(), bufferSize);

    // TODO: implement buffer device addresses so that you can launch multiple compute shaders+
    VkDescriptorSetLayoutBinding jointMatrixLayout{};
    jointMatrixLayout.binding = 0;
    jointMatrixLayout.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    jointMatrixLayout.descriptorCount = 1;
    jointMatrixLayout.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    jointMatrixLayout.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding inputVertexBufferLayout{};
    inputVertexBufferLayout.binding = 1;
    inputVertexBufferLayout.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    inputVertexBufferLayout.descriptorCount = 1;
    inputVertexBufferLayout.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    inputVertexBufferLayout.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding outputVertexBufferLayout{};
    outputVertexBufferLayout.binding = 2;
    outputVertexBufferLayout.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    outputVertexBufferLayout.descriptorCount = 1;
    outputVertexBufferLayout.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    outputVertexBufferLayout.pImmutableSamplers = nullptr;

    std::vector<VkDescriptorSetLayoutBinding> bindings = { jointMatrixLayout, inputVertexBufferLayout, outputVertexBufferLayout };

    VkDescriptorSetLayoutCreateInfo layoutCInfo{};
    layoutCInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCInfo.bindingCount = 3;
    layoutCInfo.pBindings = bindings.data();

    vkCreateDescriptorSetLayout(device_, &layoutCInfo, nullptr, &computeDescriptorSetLayout_);

    VkPushConstantRange pcRange{};
    pcRange.offset = 0;
    pcRange.size = sizeof(ComputePushConstant);
    pcRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkPipelineLayoutCreateInfo pipeLineLayoutCInfo{};
    pipeLineLayoutCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeLineLayoutCInfo.setLayoutCount = 1;
    pipeLineLayoutCInfo.pSetLayouts = &computeDescriptorSetLayout_;
    pipeLineLayoutCInfo.pushConstantRangeCount = 1;
    pipeLineLayoutCInfo.pPushConstantRanges = &pcRange;

    if (vkCreatePipelineLayout(device_, &pipeLineLayoutCInfo, nullptr, &(computePipelineLayout)) != VK_SUCCESS) {
        std::cout << "nah you buggin on dis compute shit" << std::endl;
        std::_Xruntime_error("Failed to create brdfLUT pipeline layout!");
    }

    VkDescriptorSetAllocateInfo allocateInfo{};
    allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocateInfo.descriptorPool = descriptorPool_;
    allocateInfo.descriptorSetCount = 1;
    allocateInfo.pSetLayouts = &computeDescriptorSetLayout_;
    
    VkResult res2 = vkAllocateDescriptorSets(device_, &allocateInfo, &computeDescriptorSet_);

    VkDescriptorBufferInfo skinMatrixDescriptorBufferInfo{};
    skinMatrixDescriptorBufferInfo.buffer = skinBindMatricsBuffer;
    skinMatrixDescriptorBufferInfo.offset = 0;
    skinMatrixDescriptorBufferInfo.range = (sizeof(glm::mat4) * inverseBindMatrices.size());

    VkWriteDescriptorSet skinMatrixWriteSet{};
    skinMatrixWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    skinMatrixWriteSet.dstSet = computeDescriptorSet_;
    skinMatrixWriteSet.dstBinding = 0;
    skinMatrixWriteSet.dstArrayElement = 0;
    skinMatrixWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    skinMatrixWriteSet.descriptorCount = 1;
    skinMatrixWriteSet.pBufferInfo = &skinMatrixDescriptorBufferInfo;

    VkDescriptorBufferInfo vertexDescriptorBufferInfo{};
    vertexDescriptorBufferInfo.buffer = (*animatedObjects)[0]->vertexBuffer_;
    vertexDescriptorBufferInfo.offset = 0;
    vertexDescriptorBufferInfo.range = (sizeof(Vertex) * (*animatedObjects)[0]->renderTarget->totalVertices_);

    VkWriteDescriptorSet vertexInputWriteSet{};
    vertexInputWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    vertexInputWriteSet.dstSet = computeDescriptorSet_;
    vertexInputWriteSet.dstBinding = 1;
    vertexInputWriteSet.dstArrayElement = 0;
    vertexInputWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    vertexInputWriteSet.descriptorCount = 1;
    vertexInputWriteSet.pBufferInfo = &vertexDescriptorBufferInfo;




    VkDescriptorBufferInfo vertexOutputDescriptorBufferInfo{};
    vertexOutputDescriptorBufferInfo.buffer = vertexBuffer_;
    vertexOutputDescriptorBufferInfo.offset = (sizeof(Vertex) * (*animatedObjects)[0]->renderTarget->globalFirstVertex);
    vertexOutputDescriptorBufferInfo.range = (sizeof(Vertex) * (*animatedObjects)[0]->renderTarget->totalVertices_);

    VkWriteDescriptorSet vertexOutputWriteSet{};
    vertexOutputWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    vertexOutputWriteSet.dstSet = computeDescriptorSet_;
    vertexOutputWriteSet.dstBinding = 2;
    vertexOutputWriteSet.dstArrayElement = 0;
    vertexOutputWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    vertexOutputWriteSet.descriptorCount = 1;
    vertexOutputWriteSet.pBufferInfo = &vertexOutputDescriptorBufferInfo;

    std::array<VkWriteDescriptorSet, 3> descriptors = { skinMatrixWriteSet, vertexInputWriteSet, vertexOutputWriteSet };

    vkUpdateDescriptorSets(device_, 3, descriptors.data(), 0, NULL);

    // COMPUTE PIPELINE CREATION

    VulkanPipelineBuilder::VulkanShaderModule compute = VulkanPipelineBuilder::VulkanShaderModule(device_, "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/shaders/spv/computeSkin.spv");

    VkPipelineShaderStageCreateInfo computeStageCInfo{};
    computeStageCInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computeStageCInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeStageCInfo.module = compute.module;
    computeStageCInfo.pName = "main";

    VkComputePipelineCreateInfo computePipelineCInfo{};
    computePipelineCInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineCInfo.stage = computeStageCInfo;

    computePipelineCInfo.layout = computePipelineLayout;

    vkCreateComputePipelines(device_, VK_NULL_HANDLE, 1, &computePipelineCInfo, nullptr, &computePipeline);

    // BARRIER OBJECTS
    VkCommandPoolCreateInfo computePoolCInfo = {};
    computePoolCInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    computePoolCInfo.queueFamilyIndex = QFIndices_.computeFamily.value();
    computePoolCInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vkCreateCommandPool(device_, &computePoolCInfo, nullptr, &computePool_);

    VkCommandBufferAllocateInfo computeBufferAllocationInfo{};
    computeBufferAllocationInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    computeBufferAllocationInfo.commandPool = computePool_;
    computeBufferAllocationInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    computeBufferAllocationInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device_, &computeBufferAllocationInfo, &computeBuffer_) != VK_SUCCESS) {
        std::_Xruntime_error("Failed to allocate compute command buffer!");
    }

    VkSemaphoreCreateInfo semaCInfo{};
    semaCInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceCInfo{};
    fenceCInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    vkCreateFence(device_, &fenceCInfo, nullptr, &computeFence);
    vkCreateSemaphore(device_, &semaCInfo, nullptr, &computeSema);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
SWAPCHAIN RECREATION
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderer::cleanupSWChain() {
    vkDestroyImageView(device_, depthImageView_, nullptr);
    vkDestroyImage(device_, depthImage_, nullptr);
    vkFreeMemory(device_, depthImageMemory_, nullptr);

    vkDestroyImageView(device_, colorImageView_, nullptr);
    vkDestroyImage(device_, colorImage_, nullptr);
    vkFreeMemory(device_, colorImageMemory_, nullptr);

    vkDestroyImageView(device_, bloomImageView_, nullptr);
    vkDestroyImage(device_, bloomImage_, nullptr);
    vkFreeMemory(device_, bloomImageMemory_, nullptr);

    vkDestroyImageView(device_, resolveImageView_, nullptr);
    vkDestroyImage(device_, resolveImage_, nullptr);
    vkFreeMemory(device_, resolveImageMemory_, nullptr);

    vkDestroyImageView(device_, bloomResolveImageView_, nullptr);
    vkDestroyImage(device_, bloomResolveImage_, nullptr);
    vkFreeMemory(device_, bloomResolveImageMemory_, nullptr);

    for (size_t i = 0; i < SWChainFrameBuffers_.size(); i++) {
        vkDestroyFramebuffer(device_, SWChainFrameBuffers_[i], nullptr);
    }

    for (size_t i = 0; i < SWChainImageViews_.size(); i++) {
        vkDestroyImageView(device_, SWChainImageViews_[i], nullptr);
    }

    vkDestroySwapchainKHR(device_, swapChain_, nullptr);
}

void VulkanRenderer::recreateSwapChain(SDL_Window* window) {
    int width = 0, height = 0;
    SDL_GL_GetDrawableSize(window, &width, &height);
    while (width == 0 || height == 0) {
        SDL_GL_GetDrawableSize(window, &width, &height);
    }
    vkDeviceWaitIdle(device_);

    cleanupSWChain();

    createSWChain(window);
    createImageViews(); 
    createColorResources();
    createDepthResources();
    createFrameBuffer();

    delete bloomHelper;

    bloomHelper = new BloomHelper(pDevHelper_);
    bloomHelper->setupBloom(&bloomResolveImage_, &bloomResolveImageView_, VK_FORMAT_R16G16B16A16_SFLOAT, SWChainExtent_);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
FREEING
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void VulkanRenderer::shutdown() {
    cleanupSWChain();

    delete brdfLut;
    delete irCube;
    delete prefEMap;
    delete opaquePipeline_;
    delete prepassPipeline_;
    delete toonPipeline_;
    delete outlinePipeline_;
    delete toneMappingPipeline_;

    delete pDirectionalLight_;

    vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);

    vkDestroyCommandPool(device_, commandPool_, nullptr);

    vkDestroyRenderPass(device_, renderPass_, nullptr);
    vkDestroyRenderPass(device_, toneMapPass_, nullptr);
    vkDestroyRenderPass(device_, depthPrepass_, nullptr);

    delete pDevHelper_;

    vkDestroyDevice(device_, nullptr);

    if (enableValLayers) {
        DestroyDebugUtilsMessengerEXT(instance_, debugMessenger_, nullptr);
    }

    vkDestroyInstance(instance_, nullptr);
}