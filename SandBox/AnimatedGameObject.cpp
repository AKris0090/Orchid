#include "AnimatedGameObject.h"
#include "Time.h"

glm::mat4 AnimatedGameObject::getNodeMatrix(AnimSceneNode* node)
{
    glm::mat4 nodeMatrix = node->getAnimatedMatrix();
    AnimSceneNode* currentParent = node->parent;
    while (currentParent)
    {
        nodeMatrix = currentParent->getAnimatedMatrix() * nodeMatrix;
        currentParent = currentParent->parent;
    }
    return nodeMatrix;
}

void AnimatedGameObject::updateJoints(AnimSceneNode* node, std::vector<glm::mat4>& bindMatrices) {
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

using namespace std::literals;

void AnimatedGameObject::smoothFromCurrentPosition(std::vector<glm::mat4>& bindMatrices, float deltaTime) {
    activeAnimation->currentTime += deltaTime;
    previousAnimation->currentTime += deltaTime;
    if (activeAnimation->currentTime > activeAnimation->end)
    {
        activeAnimation->currentTime -= activeAnimation->end;
    }
    if (previousAnimation->currentTime > previousAnimation->end)
    {
        previousAnimation->currentTime -= previousAnimation->end;
    }

    std::vector<glm::vec3> srcPos = std::vector<glm::vec3>(previousAnimation->channels.size());
    std::vector<glm::quat> srcQuat = std::vector<glm::quat>(previousAnimation->channels.size());
    std::vector<glm::vec3> srcScale = std::vector<glm::vec3>(previousAnimation->channels.size());

    int count = 0;

    for (auto& channel : previousAnimation->channels)
    {
        Animation::AnimationSampler& sampler = previousAnimation->samplers[channel.samplerIndex];
        for (size_t i = 0; i < sampler.inputs.size() - 1; i++)
        {
            if (sampler.interpolation != "LINEAR")
            {
                std::cout << "This sample only supports linear interpolations\n";
                continue;
            }

            // Get the input keyframe values for the current time stamp
            if ((previousAnimation->currentTime >= sampler.inputs[i]) && (previousAnimation->currentTime <= sampler.inputs[i + 1]))
            {
                float a = (previousAnimation->currentTime - sampler.inputs[i]) / (sampler.inputs[i + 1] - sampler.inputs[i]);
                if (channel.path == "translation")
                {
                    srcPos[count] = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], a);
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

                    srcQuat[count] = glm::normalize(glm::slerp(q1, q2, a));
                }
                if (channel.path == "scale")
                {
                    srcScale[count] = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], a);
                }
            }
        }
        count++;
    }

    count = 0;
    for (auto& channel : activeAnimation->channels)
    {
        Animation::AnimationSampler& sampler = activeAnimation->samplers[channel.samplerIndex];
        for (size_t i = 0; i < sampler.inputs.size() - 1; i++)
        {
            if (sampler.interpolation != "LINEAR")
            {
                std::cout << "This sample only supports linear interpolations\n";
                continue;
            }

            // Get the input keyframe values for the current time stamp
            if ((activeAnimation->currentTime >= sampler.inputs[i]) && (activeAnimation->currentTime <= sampler.inputs[i + 1]))
            {
                float a = (activeAnimation->currentTime - sampler.inputs[i]) / (sampler.inputs[i + 1] - sampler.inputs[i]);
                if (channel.path == "translation")
                {
                    glm::vec3 dstPos = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], a);

                    channel.node->translation = Time::weightLerp(srcPos[count], dstPos, smoothAmount);
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

                    glm::quat dstQuat = glm::normalize(glm::slerp(q1, q2, a));

                    channel.node->rotation.x = Time::weightLerp(srcQuat[count].x, dstQuat.x, smoothAmount);
                    channel.node->rotation.y = Time::weightLerp(srcQuat[count].y, dstQuat.y, smoothAmount);
                    channel.node->rotation.z = Time::weightLerp(srcQuat[count].z, dstQuat.z, smoothAmount);
                    channel.node->rotation.w = Time::weightLerp(srcQuat[count].w, dstQuat.w, smoothAmount);
                }
                if (channel.path == "scale")
                {
                    glm::vec3 dstScale = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], a);

                    channel.node->scale = Time::weightLerp(srcScale[count], dstScale, smoothAmount);
                }
            }
        }
        count++;
    }
}

void AnimatedGameObject::updateAnimation(std::vector<glm::mat4>& bindMatrices, float deltaTime) {
    if (needsSmooth) {
        smoothStart = Time::getCurrentTime();
        smoothUntil = smoothStart + smoothDuration;
        smoothAmount = 0.0f;
        activeAnimation->currentTime = 0.0f;
        needsSmooth = false;
    }
    if (smoothAmount <= 1.0f) {
        smoothAmount = (std::chrono::duration<float>(Time::getCurrentTime() - smoothStart) / smoothDuration);
        smoothFromCurrentPosition(bindMatrices, deltaTime);
        for (auto& node : renderTarget->pParentNodes)
        {
            updateJoints(node, bindMatrices);
        }
        return;
    }
    Animation* animation = activeAnimation;
    animation->currentTime += deltaTime;
    if (animation->currentTime > animation->end)
    {
        animation->currentTime -= animation->end;
    }

    for (auto& channel : animation->channels)
    {
        Animation::AnimationSampler& sampler = animation->samplers[channel.samplerIndex];
        for (size_t i = 0; i < sampler.inputs.size() - 1; i++)
        {
            if (sampler.interpolation != "LINEAR")
            {
                std::cout << "This sample only supports linear interpolations\n";
                continue;
            }

            // Get the input keyframe values for the current time stamp
            if ((animation->currentTime >= sampler.inputs[i]) && (animation->currentTime <= sampler.inputs[i + 1]))
            {
                float a = (animation->currentTime - sampler.inputs[i]) / (sampler.inputs[i + 1] - sampler.inputs[i]);
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
    vkMapMemory(pDevHelper->device_, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, basePoseVertices_.data(), (size_t)bufferSize);
    vkUnmapMemory(pDevHelper->device_, stagingBufferMemory);

    pDevHelper->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer_, vertexBufferMemory_);
    pDevHelper->copyBuffer(stagingBuffer, this->vertexBuffer_, bufferSize);

    vkDestroyBuffer(pDevHelper->device_, stagingBuffer, nullptr);
    vkFreeMemory(pDevHelper->device_, stagingBufferMemory, nullptr);
}