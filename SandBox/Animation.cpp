#include "Animation.h"

AnimSceneNode* findNode(AnimSceneNode* parent, uint32_t index)
{
    AnimSceneNode* nodeFound = nullptr;
    if (parent->index == index)
    {
        return parent;
    }
    for (auto& child : parent->children)
    {
        nodeFound = findNode(child, index);
        if (nodeFound)
        {
            break;
        }
    }
    return nodeFound;
}

AnimSceneNode* nodeFromIndex(uint32_t index, std::vector<AnimSceneNode*> pParentNodes)
{
    AnimSceneNode* nodeFound = nullptr;
    for (auto& node : pParentNodes)
    {
        nodeFound = findNode(node, index);
        if (nodeFound)
        {
            break;
        }
    }
    return nodeFound;
}

void Animation::loadAnimation(std::string gltfPath_, std::vector<AnimSceneNode*> pParentNodes) {
    tinygltf::Model in;
    tinygltf::TinyGLTF gltfContext;
    std::string error, warning;

    bool loadedFile = false;
    std::filesystem::path fPath = gltfPath_;
    if (fPath.extension() == ".glb") {
        loadedFile = gltfContext.LoadBinaryFromFile(&(in), &error, &warning, gltfPath_);
    }
    else if (fPath.extension() == ".gltf") {
        loadedFile = gltfContext.LoadASCIIFromFile(&(in), &error, &warning, gltfPath_);
    }

    for (size_t i = 0; i < in.animations.size(); i++)
    {
        tinygltf::Animation glTFAnimation = in.animations[i];
        name = glTFAnimation.name;

        // Samplers
        samplers.resize(glTFAnimation.samplers.size());
        for (size_t j = 0; j < glTFAnimation.samplers.size(); j++)
        {
            tinygltf::AnimationSampler glTFSampler = glTFAnimation.samplers[j];
            Animation::AnimationSampler& dstSampler = samplers[j];
            dstSampler.interpolation = glTFSampler.interpolation;

            // Read sampler keyframe input time values
            {
                const tinygltf::Accessor& accessor = in.accessors[glTFSampler.input];
                const tinygltf::BufferView& bufferView = in.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = in.buffers[bufferView.buffer];
                const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
                const float* buf = static_cast<const float*>(dataPtr);
                for (size_t index = 0; index < accessor.count; index++)
                {
                    dstSampler.inputs.push_back(buf[index]);
                }
                // Adjust animation's start and end times
                for (auto input : samplers[j].inputs)
                {
                    if (input < start)
                    {
                        start = input;
                    };
                    if (input > end)
                    {
                        end = input;
                    }
                }
            }

            // Read sampler keyframe output translate/rotate/scale values
            {
                const tinygltf::Accessor& accessor = in.accessors[glTFSampler.output];
                const tinygltf::BufferView& bufferView = in.bufferViews[accessor.bufferView];
                const tinygltf::Buffer& buffer = in.buffers[bufferView.buffer];
                const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
                switch (accessor.type)
                {
                case TINYGLTF_TYPE_VEC3: {
                    const glm::vec3* buf = static_cast<const glm::vec3*>(dataPtr);
                    for (size_t index = 0; index < accessor.count; index++)
                    {
                        dstSampler.outputsVec4.push_back(glm::vec4(buf[index], 0.0f));
                    }
                    break;
                }
                case TINYGLTF_TYPE_VEC4: {
                    const glm::vec4* buf = static_cast<const glm::vec4*>(dataPtr);
                    for (size_t index = 0; index < accessor.count; index++)
                    {
                        dstSampler.outputsVec4.push_back(buf[index]);
                    }
                    break;
                }
                default: {
                    std::cout << "unknown type" << std::endl;
                    break;
                }
                }
            }
        }

        // Channels
        channels.resize(glTFAnimation.channels.size());
        for (size_t j = 0; j < glTFAnimation.channels.size(); j++)
        {
            tinygltf::AnimationChannel glTFChannel = glTFAnimation.channels[j];
            Animation::AnimationChannel& dstChannel = channels[j];
            dstChannel.path = glTFChannel.target_path;
            dstChannel.samplerIndex = glTFChannel.sampler;
            dstChannel.node = nodeFromIndex(glTFChannel.target_node, pParentNodes);
        }
    }
}