#include "AnimatedGameObject.h"

glm::mat4 AnimatedGameObject::getNodeMatrix(AnimatedGLTFObj::SceneNode* node)
{
    glm::mat4 nodeMatrix = node->getAnimatedMatrix();
    AnimatedGLTFObj::SceneNode* currentParent = node->parent;
    while (currentParent)
    {
        nodeMatrix = currentParent->getAnimatedMatrix() * nodeMatrix;
        currentParent = currentParent->parent;
    }
    return nodeMatrix;
}

void AnimatedGameObject::updateJoints(AnimatedGLTFObj::SceneNode* node, std::vector<glm::mat4>& bindMatrices) {
    if (node->skinIndex > -1)
    {
        // Update the joint matrices
        glm::mat4              inverseTransform = glm::inverse(getNodeMatrix(node));
        AnimatedGLTFObj::Skin  skin = renderTarget->skins_[node->skinIndex];
        size_t                 numJoints = (uint32_t)skin.joints.size();
        std::vector<glm::mat4> jointMatrices(numJoints);
        for (size_t i = renderTarget->globalSkinningMatrixOffset; i < renderTarget->globalSkinningMatrixOffset + numJoints; i++)
        {
            bindMatrices[i] = inverseTransform * (getNodeMatrix(skin.joints[i]) * skin.inverseBindMatrices[i]);
        }
    }

    for (auto& child : node->children)
    {
        updateJoints(child, bindMatrices);
    }
}

void AnimatedGameObject::updateAnimation(std::vector<glm::mat4>& bindMatrices, float deltaTime) {
    if (renderTarget->activeAnimation > static_cast<uint32_t>(renderTarget->animations_.size()) - 1)
    {
        std::cout << "No animation with index " << renderTarget->activeAnimation << std::endl;
        return;
    }
    AnimatedGLTFObj::Animation& animation = renderTarget->animations_[renderTarget->activeAnimation];
    animation.currentTime += deltaTime;
    if (animation.currentTime > animation.end)
    {
        animation.currentTime -= animation.end;
    }

    for (auto& channel : animation.channels)
    {
        AnimatedGLTFObj::AnimationSampler& sampler = animation.samplers[channel.samplerIndex];
        for (size_t i = 0; i < sampler.inputs.size() - 1; i++)
        {
            if (sampler.interpolation != "LINEAR")
            {
                std::cout << "This sample only supports linear interpolations\n";
                continue;
            }

            // Get the input keyframe values for the current time stamp
            if ((animation.currentTime >= sampler.inputs[i]) && (animation.currentTime <= sampler.inputs[i + 1]))
            {
                float a = (animation.currentTime - sampler.inputs[i]) / (sampler.inputs[i + 1] - sampler.inputs[i]);
                if (channel.path == "translation")
                {
                    channel.node->translation = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], a);
                }
                if (channel.path == "rotation")
                {
                    glm::quat q1;
                    q1.x = sampler.outputsVec4[i].x;
                    q1.y = sampler.outputsVec4[i].y;
                    q1.z = sampler.outputsVec4[i].z;
                    q1.w = sampler.outputsVec4[i].w;

                    glm::quat q2;
                    q2.x = sampler.outputsVec4[i + 1].x;
                    q2.y = sampler.outputsVec4[i + 1].y;
                    q2.z = sampler.outputsVec4[i + 1].z;
                    q2.w = sampler.outputsVec4[i + 1].w;

                    channel.node->rotation = glm::normalize(glm::slerp(q1, q2, a));
                }
                if (channel.path == "scale")
                {
                    channel.node->scale = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], a);
                }
            }
        }
    }
    for (auto& node : renderTarget->pParentNodes)
    {
        updateJoints(node, bindMatrices);
    }
}

void AnimatedGameObject::createVertexBuffer() {
    VkDeviceSize bufferSize = sizeof(Vertex) * basePoseVertices_.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    pDevHelper->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device_, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, basePoseVertices_.data(), (size_t)bufferSize);
    vkUnmapMemory(device_, stagingBufferMemory);

    pDevHelper->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer_, vertexBufferMemory_);
    pDevHelper->copyBuffer(stagingBuffer, this->vertexBuffer_, bufferSize);

    vkDestroyBuffer(device_, stagingBuffer, nullptr);
    vkFreeMemory(device_, stagingBufferMemory, nullptr);
}

void AnimatedGameObject::createSkinnedBuffer() {
    VkDeviceSize bufferSize = sizeof(Vertex) * basePoseVertices_.size();

    pDevHelper->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, skinnedBuffer_, skinnedBufferDeviceMemory_);
}

void AnimatedGameObject::createIndexBuffer() {
    VkDeviceSize bufferSize = sizeof(basePoseIndices_[0]) * basePoseIndices_.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    pDevHelper->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device_, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, basePoseIndices_.data(), (size_t)bufferSize);
    vkUnmapMemory(device_, stagingBufferMemory);

    pDevHelper->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer_, indexBufferMemory_);
    pDevHelper->copyBuffer(stagingBuffer, indexBuffer_, bufferSize);

    vkDestroyBuffer(device_, stagingBuffer, nullptr);
    vkFreeMemory(device_, stagingBufferMemory, nullptr);
}