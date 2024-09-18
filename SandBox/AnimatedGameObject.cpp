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

void AnimatedGameObject::getAnimatedNodeTransform(std::vector<AnimatedGameObject::secondaryTransform>* transforms, Animation* anim) {
    int count = 0;

    for (auto& channel : anim->channels)
    {
        Animation::AnimationSampler& sampler = anim->samplers[channel.samplerIndex];
        for (size_t i = 0; i < sampler.inputs.size() - 1; i++)
        {
            if (sampler.interpolation != "LINEAR")
            {
                std::cout << "This sample only supports linear interpolations\n";
                continue;
            }

            // Get the input keyframe values for the current time stamp
            if ((anim->currentTime >= sampler.inputs[i]) && (anim->currentTime <= sampler.inputs[i + 1]))
            {
                float a = (anim->currentTime - sampler.inputs[i]) / (sampler.inputs[i + 1] - sampler.inputs[i]);
                if (channel.path == "translation")
                {
                    transforms->at(count).position = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], a);
                }
                if (channel.path == "rotation")
                {
                    glm::quat q1{};
                    q1.x = sampler.outputsVec4[i].x;
                    q1.y = sampler.outputsVec4[i].y;
                    q1.z = sampler.outputsVec4[i].z;
                    q1.w = sampler.outputsVec4[i].w;

                    glm::quat q2{};
                    q2.x = sampler.outputsVec4[i + 1].x;
                    q2.y = sampler.outputsVec4[i + 1].y;
                    q2.z = sampler.outputsVec4[i + 1].z;
                    q2.w = sampler.outputsVec4[i + 1].w;

                    transforms->at(count).rotation = glm::normalize(glm::slerp(q1, q2, a));
                }
                if (channel.path == "scale")
                {
                    transforms->at(count).scale = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], a);
                }
            }
        }
        count++;
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

    getAnimatedNodeTransform(src, previousAnimation);
    getAnimatedNodeTransform(dst, activeAnimation);

    int i = 0;

    for (auto& channel : activeAnimation->channels) {
        switch (hash_str(channel.path)) {
        case STRINGENUM::TRANSLATION:
            channel.node->translation = Time::weightLerp(src->at(i).position, dst->at(i).position, smoothAmount);
            break;
        case STRINGENUM::ROTATION:
            channel.node->rotation.x = Time::weightLerp(src->at(i).rotation.x, dst->at(i).rotation.x, smoothAmount);
            channel.node->rotation.y = Time::weightLerp(src->at(i).rotation.y, dst->at(i).rotation.y, smoothAmount);
            channel.node->rotation.z = Time::weightLerp(src->at(i).rotation.z, dst->at(i).rotation.z, smoothAmount);
            channel.node->rotation.w = Time::weightLerp(src->at(i).rotation.w, dst->at(i).rotation.w, smoothAmount);
            break;
        case STRINGENUM::SCALE:
            channel.node->scale = Time::weightLerp(src->at(i).scale, dst->at(i).scale, smoothAmount);
            break;
        default:
            break;
        }
        i++;
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

    getAnimatedNodeTransform(dst, animation);
    int i = 0;
    for (auto& channel : animation->channels)
    {
        switch (hash_str(channel.path)) {
        case STRINGENUM::TRANSLATION:
            channel.node->translation = dst->at(i).position;
            break;
        case STRINGENUM::ROTATION:
            channel.node->rotation = dst->at(i).rotation;
            break;
        case STRINGENUM::SCALE:
            channel.node->scale = dst->at(i).scale;
            break;
        default:
            break;
        }
        i++;
    }
    for (auto& node : renderTarget->pParentNodes)
    {
        updateJoints(node, bindMatrices);
    }
}