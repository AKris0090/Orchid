#include "AnimatedGLTFObj.h"

#include "GLTFObject.h"

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

// MIKKTSPACE TANGENT FUNCTIONS, REFERENCED OFF OF: https://github.com/Eearslya/glTFView. SUCH AN AMAZING PERSON
struct MikkTContext {
    MeshHelper* mesh;
};

static int MikkTGetNumFaces(const SMikkTSpaceContext* context) {
    const auto data = reinterpret_cast<const MikkTContext*>(context->m_pUserData);
    return data->mesh->vertices_.size() / 3;
}

static int MikkTGetNumVerticesOfFace(const SMikkTSpaceContext* context, const int face) {
    return 3;
}

static void MikkTGetPosition(const SMikkTSpaceContext* context, float fvPosOut[], const int face, const int vert) {
    const auto data = reinterpret_cast<const MikkTContext*>(context->m_pUserData);
    const glm::vec3 pos = data->mesh->vertices_[face * 3 + vert].pos;
    fvPosOut[0] = pos.x;
    fvPosOut[1] = pos.y;
    fvPosOut[2] = pos.z;
}

static void MikkTGetNormal(const SMikkTSpaceContext* context, float fvNormOut[], const int face, const int vert) {
    const auto data = reinterpret_cast<const MikkTContext*>(context->m_pUserData);
    const glm::vec3 norm = data->mesh->vertices_[face * 3 + vert].normal;
    fvNormOut[0] = norm.x;
    fvNormOut[1] = norm.y;
    fvNormOut[2] = norm.z;
}

static void MikkTGetTexCoord(const SMikkTSpaceContext* context, float fvTexcOut[], const int face, const int vert) {
    const auto data = reinterpret_cast<const MikkTContext*>(context->m_pUserData);
    glm::vec2 uv = data->mesh->vertices_[face * 3 + vert].uv;
    fvTexcOut[0] = uv.x;
    fvTexcOut[1] = 1.0 - uv.y;
}

static void MikkTSetTSpaceBasic(
    const SMikkTSpaceContext* context, const float fvTangent[], const float fSign, const int face, const int vert) {
    auto data = reinterpret_cast<MikkTContext*>(context->m_pUserData);

    data->mesh->vertices_[face * 3 + vert].tangent = glm::vec4(glm::make_vec3(fvTangent), fSign);
}

static SMikkTSpaceInterface MikkTInterface = { .m_getNumFaces = MikkTGetNumFaces,
                                              .m_getNumVerticesOfFace = MikkTGetNumVerticesOfFace,
                                              .m_getPosition = MikkTGetPosition,
                                              .m_getNormal = MikkTGetNormal,
                                              .m_getTexCoord = MikkTGetTexCoord,
                                              .m_setTSpaceBasic = MikkTSetTSpaceBasic,
                                              .m_setTSpace = nullptr };

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AnimatedGLTFObj::drawIndexedOpaque(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) {
    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &(modelTransform));
    for (auto& mat : opaqueDraws) {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &(mat.first->descriptorSet), 0, nullptr);
        for (auto& draw : mat.second) {
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1, draw->skinSet, 0, nullptr);
            vkCmdDrawIndexed(commandBuffer, draw->numIndices, 1, draw->firstIndex, 0, 0);
        }
    }
}

void AnimatedGLTFObj::drawIndexedTransparent(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) {
    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &(modelTransform));
    for (auto& mat : transparentDraws) {
        MeshHelper::Material* material = mat.first;
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &(material->descriptorSet), 0, nullptr);
        pcBlock newBlock{ (material->alphaMode == "MASK"), material->alphaCutOff };
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(pcBlock), &(newBlock));
        for (auto& draw : mat.second) {
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 2, 1, draw->skinSet, 0, nullptr);
            vkCmdDrawIndexed(commandBuffer, draw->numIndices, 1, draw->firstIndex, 0, 0);
        }
    }
}

void AnimatedGLTFObj::render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) {
    VkBuffer vertexBuffers[] = { pSceneMesh_->getVertexBuffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, pSceneMesh_->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
}

void AnimatedGLTFObj::drawShadow(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VkPipeline animatedShadowPipeline, uint32_t cascadeIndex, VkDescriptorSet cascadeDescriptor, SceneNode* node) {
    if (node->mesh.primitives.size() > 0) {
        glm::mat4 nodeTransform = modelTransform;
        cascadeBlock.model = nodeTransform;
        if (node->skin >= 0) {
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &(skins_[node->skin].descriptorSet), 0, nullptr);
        }
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &cascadeDescriptor, 0, nullptr);
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(cascadeMVP), &cascadeBlock);
        for (MeshHelper::PrimitiveObjIndices p : node->mesh.primitives) {
            if (p.numIndices > 0) {
                vkCmdDrawIndexed(commandBuffer, p.numIndices, 1, p.firstIndex, 0, 0);
            }
        }
    }
    for (auto& child : node->children) {
        drawShadow(commandBuffer, pipelineLayout, animatedShadowPipeline, cascadeIndex, cascadeDescriptor, child);
    }
}

void AnimatedGLTFObj::renderShadow(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, VkPipeline animatedShadowPipeline, uint32_t cascadeIndex, VkDescriptorSet cascadeDescriptor) {
    VkBuffer vertexBuffers[] = { pSceneMesh_->getVertexBuffer() };
    VkDeviceSize offsets[] = { 0 };
    cascadeBlock.cascadeIndex = cascadeIndex;
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, pSceneMesh_->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

    for (auto& node : pNodes_) {
        drawShadow(commandBuffer, pipelineLayout, animatedShadowPipeline, cascadeIndex, cascadeDescriptor, node);
    }
}

glm::mat4 AnimatedGLTFObj::SceneNode::getLocalMatrix() {
    return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale) * matrix;
}

// From: Sascha Willems' gltfskinning example
void AnimatedGLTFObj::updateAnimation(float deltaTime)
{
    if (activeAnimation > static_cast<uint32_t>(animations_.size()) - 1)
    {
        std::cout << "No animation with index " << activeAnimation << std::endl;
        return;
    }
    Animation& animation = animations_[activeAnimation];
    animation.currentTime += deltaTime;
    if (animation.currentTime > animation.end)
    {
        animation.currentTime -= animation.end;
    }

    for (auto& channel : animation.channels)
    {
        AnimationSampler& sampler = animation.samplers[channel.samplerIndex];
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
    for (auto& node : pNodes_)
    {
        updateJoints(node);
    }
}

// Also from Sascha Willems' gltfskinning example
glm::mat4 AnimatedGLTFObj::getNodeMatrix(SceneNode* node)
{
    glm::mat4              nodeMatrix = node->getLocalMatrix();
    SceneNode* currentParent = node->parent;
    while (currentParent)
    {
        nodeMatrix = currentParent->getLocalMatrix() * nodeMatrix;
        currentParent = currentParent->parent;
    }
    return nodeMatrix;
}

// Also from Sascha Willems' gltfskinning example
void AnimatedGLTFObj::updateJoints(SceneNode* node)
{
    if (node->skin > -1)
    {
        // Update the joint matrices
        glm::mat4              inverseTransform = glm::inverse(getNodeMatrix(node));
        Skin                   skin = skins_[node->skin];
        size_t                 numJoints = (uint32_t)skin.joints.size();
        std::vector<glm::mat4> jointMatrices(numJoints);
        for (size_t i = 0; i < numJoints; i++)
        {
            jointMatrices[i] = inverseTransform * (getNodeMatrix(skin.joints[i]) * skin.inverseBindMatrices[i]);
        }

        // Update ssbo
        memcpy(skin.ssbo, jointMatrices.data(), jointMatrices.size() * sizeof(glm::mat4));
    }

    for (auto& child : node->children)
    {
        updateJoints(child);
    }
}

// LOAD FUNCTIONS TEMPLATED FROM GLTFLOADING EXAMPLE ON GITHUB BY SASCHA WILLEMS
void AnimatedGLTFObj::loadImages(tinygltf::Model& in, std::vector<TextureHelper*>& images) {
    for (size_t i = 0; i < in.images.size(); i++) {
        TextureHelper* tex = new TextureHelper(in, int(i), pDevHelper_);
        tex->load();
        pSceneMesh_->images_.push_back(tex);
    }
    TextureHelper* dummyAO = new TextureHelper(in, -1, pDevHelper_);
    TextureHelper* dummyMetallic = new TextureHelper(in, -2, pDevHelper_);
    TextureHelper* dummyNormal = new TextureHelper(in, -3, pDevHelper_);
    TextureHelper* dummyEmission = new TextureHelper(in, -4, pDevHelper_);
    dummyAO->load();
    dummyMetallic->load();
    dummyNormal->load();
    dummyEmission->load();
    pSceneMesh_->images_.push_back(dummyNormal);
    pSceneMesh_->images_.push_back(dummyMetallic);
    pSceneMesh_->images_.push_back(dummyAO);
    pSceneMesh_->images_.push_back(dummyEmission);

    std::cout << std::endl << "loaded: " << in.images.size() << "  images" << std::endl;
}

void AnimatedGLTFObj::loadTextures(tinygltf::Model& in, std::vector<TextureHelper::TextureIndexHolder>& textures) {
    pSceneMesh_->textures_.resize(in.textures.size());
    uint32_t size = static_cast<uint32_t>(textures.size());
    for (size_t i = 0; i < in.textures.size(); i++) {
        pSceneMesh_->textures_[i].textureIndex = in.textures[i].source;
    }
    pSceneMesh_->textures_.push_back(TextureHelper::TextureIndexHolder{ size });
    pSceneMesh_->textures_.push_back(TextureHelper::TextureIndexHolder{ size + 1 });
    pSceneMesh_->textures_.push_back(TextureHelper::TextureIndexHolder{ size + 2 });
    pSceneMesh_->textures_.push_back(TextureHelper::TextureIndexHolder{ size + 3 });
}

void AnimatedGLTFObj::loadNode(tinygltf::Model& in, const tinygltf::Node& nodeIn, uint32_t nodeIndex, SceneNode* parent, std::vector<SceneNode*>& nodes) {
    SMikkTSpaceContext mikktContext = { .m_pInterface = &MikkTInterface };

    SceneNode* scNode = new SceneNode{};
    scNode->matrix = glm::mat4(1.0f);
    scNode->skin = nodeIn.skin;
    scNode->parent = parent;
    scNode->index = nodeIndex;

    if (nodeIn.translation.size() == 3) {
        scNode->translation = glm::make_vec3(nodeIn.translation.data());
    }
    if (nodeIn.rotation.size() == 4) {
        glm::quat q = glm::make_quat(nodeIn.rotation.data());
        scNode->rotation = glm::mat4(q);
    }
    if (nodeIn.scale.size() == 3) {
        scNode->scale = glm::make_vec3(nodeIn.scale.data());
    }
    if (nodeIn.matrix.size() == 16) {
        scNode->matrix = glm::make_mat4x4(nodeIn.matrix.data());
    }

    if (nodeIn.children.size() > 0) {
        for (size_t i = 0; i < nodeIn.children.size(); i++) {
            loadNode(in, in.nodes[nodeIn.children[i]], nodeIn.children[i], scNode, nodes);
        }
    }

    if (nodeIn.mesh > -1) {
        const tinygltf::Mesh mesh = in.meshes[nodeIn.mesh];
        for (size_t i = 0; i < mesh.primitives.size(); i++) {
            const tinygltf::Primitive& gltfPrims = mesh.primitives[i];
            uint32_t firstIndex = static_cast<uint32_t>(pSceneMesh_->indices_.size());
            uint32_t vertexStart = static_cast<uint32_t>(pSceneMesh_->vertices_.size());
            uint32_t indexCount = 0;
            uint32_t numVertices = 0;

            // FOR VERTICES
            const float* positionBuff = nullptr;
            const float* normalsBuff = nullptr;
            const float* uvBuff = nullptr;
            const float* tangentsBuff = nullptr;
            const void* jointIndicesBuffer = nullptr;
            const float* jointWeightsBuffer = nullptr;

            if (gltfPrims.attributes.find("POSITION") != gltfPrims.attributes.end()) {
                const tinygltf::Accessor& accessor = in.accessors[gltfPrims.attributes.find("POSITION")->second];
                const tinygltf::BufferView& view = in.bufferViews[accessor.bufferView];
                positionBuff = reinterpret_cast<const float*>(&(in.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                numVertices = static_cast<uint32_t>(accessor.count);
            }
            if (gltfPrims.attributes.find("NORMAL") != gltfPrims.attributes.end()) {
                const tinygltf::Accessor& accessor = in.accessors[gltfPrims.attributes.find("NORMAL")->second];
                const tinygltf::BufferView& view = in.bufferViews[accessor.bufferView];
                normalsBuff = reinterpret_cast<const float*>(&(in.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
            }
            if (gltfPrims.attributes.find("TEXCOORD_0") != gltfPrims.attributes.end()) {
                const tinygltf::Accessor& accessor = in.accessors[gltfPrims.attributes.find("TEXCOORD_0")->second];
                const tinygltf::BufferView& view = in.bufferViews[accessor.bufferView];
                uvBuff = reinterpret_cast<const float*>(&(in.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
            }
            if (gltfPrims.attributes.find("TANGENT") != gltfPrims.attributes.end()) {
                const tinygltf::Accessor& accessor = in.accessors[gltfPrims.attributes.find("TANGENT")->second];
                const tinygltf::BufferView& view = in.bufferViews[accessor.bufferView];
                tangentsBuff = reinterpret_cast<const float*>(&(in.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
            }
            int jointType;
            if (gltfPrims.attributes.find("JOINTS_0") != gltfPrims.attributes.end()) {
                const tinygltf::Accessor& accessor = in.accessors[gltfPrims.attributes.find("JOINTS_0")->second];
                jointType = accessor.componentType;
                const tinygltf::BufferView& view = in.bufferViews[accessor.bufferView];
                jointIndicesBuffer = reinterpret_cast<const void*>(&(in.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
            }
            if (gltfPrims.attributes.find("WEIGHTS_0") != gltfPrims.attributes.end()) {
                const tinygltf::Accessor& accessor = in.accessors[gltfPrims.attributes.find("WEIGHTS_0")->second];
                const tinygltf::BufferView& view = in.bufferViews[accessor.bufferView];
                jointWeightsBuffer = reinterpret_cast<const float*>(&(in.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
            }

            bool hasSkin = (jointIndicesBuffer && jointWeightsBuffer);

            for (size_t vert = 0; vert < numVertices; vert++) {
                MeshHelper::Vertex v;
                v.pos = glm::vec4(glm::make_vec3(&positionBuff[vert * 3]), 1.0f);
                v.normal = glm::normalize(glm::vec3(normalsBuff ? glm::make_vec3(&normalsBuff[vert * 3]) : glm::vec3(0.0f)));
                v.uv = uvBuff ? glm::make_vec2(&uvBuff[vert * 2]) : glm::vec3(0.0f);
                v.color = glm::vec3(1.0f);
                v.tangent = tangentsBuff ? glm::make_vec4(&tangentsBuff[vert * 4]) : glm::vec4(0.0f);
                if (hasSkin) {
                    switch (jointType) {
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
                        const uint16_t* buffer = static_cast<const uint16_t*>(jointIndicesBuffer);
                        v.jointIndices = glm::vec4(glm::make_vec4(&buffer[vert * 4]));
                        break;
                    }
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
                        const uint8_t* buffer = static_cast<const uint8_t*>(jointIndicesBuffer);
                        v.jointIndices = glm::vec4(glm::make_vec4(&buffer[vert * 4]));
                        break;
                    }
                    default:
                        std::cout << "Joint component type not supported" << std::endl;
                        break;
                    }
                }
                else {
                    v.jointIndices = glm::vec4(0.0f);
                }
                v.jointWeights = hasSkin ? glm::make_vec4(&jointWeightsBuffer[vert * 4]) : glm::vec4(0.0f);
                pSceneMesh_->vertices_.push_back(v);
                totalVertices_++;
            }

            // FOR INDEX
            const tinygltf::Accessor& accessor = in.accessors[gltfPrims.indices];
            const tinygltf::BufferView& view = in.bufferViews[accessor.bufferView];
            const tinygltf::Buffer& buffer = in.buffers[view.buffer];

            indexCount += static_cast<uint32_t>(accessor.count);

            switch (accessor.componentType) {
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                const uint32_t* buf = reinterpret_cast<const uint32_t*>(&buffer.data[accessor.byteOffset + view.byteOffset]);
                for (size_t index = 0; index < accessor.count; index++) {
                    pSceneMesh_->indices_.push_back(buf[index] + vertexStart);
                }
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                const uint16_t* buf = reinterpret_cast<const uint16_t*>(&buffer.data[accessor.byteOffset + view.byteOffset]);
                for (size_t index = 0; index < accessor.count; index++) {
                    pSceneMesh_->indices_.push_back(buf[index] + vertexStart);
                }
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                const uint8_t* buf = reinterpret_cast<const uint8_t*>(&buffer.data[accessor.byteOffset + view.byteOffset]);
                for (size_t index = 0; index < accessor.count; index++) {
                    pSceneMesh_->indices_.push_back(buf[index] + vertexStart);
                }
                break;
            }
            default:
                std::cout << "index component type not supported" << std::endl;
                std::_Xruntime_error("index component type not supported");
            }
            MeshHelper::PrimitiveObjIndices p;
            p.firstIndex = firstIndex;
            p.numIndices = indexCount;
            totalIndices_ += indexCount;
            p.materialIndex = gltfPrims.material;
            scNode->mesh.primitives.push_back(p);

            // TANGENT SPACE CREATION - ALSO REFERENCED OFF OF: https://github.com/Eearslya/glTFView.
            if (!tangentsBuff) {
                totalIndices_ = 0;
                totalVertices_ = 0;
                //UNPACK VERTICES
                std::vector<MeshHelper::Vertex> unpacked(pSceneMesh_->indices_.size());
                uint32_t newInd = 0;
                for (uint32_t index : pSceneMesh_->indices_) {
                    unpacked[newInd] = pSceneMesh_->vertices_[index];
                    newInd++;
                }
                pSceneMesh_->vertices_ = std::move(unpacked);
                pSceneMesh_->indices_.clear();

                // GEN TANGENT SPACE
                MikkTContext context{ pSceneMesh_ };
                mikktContext.m_pUserData = &context;
                genTangSpaceDefault(&mikktContext);

                //WELD VERTICES
                pSceneMesh_->indices_.clear();
                pSceneMesh_->indices_.reserve(pSceneMesh_->vertices_.size());
                std::unordered_map<MeshHelper::Vertex, uint32_t> uniqueVertices;

                size_t oldVertexCount = pSceneMesh_->vertices_.size();
                uint32_t postTVertexCount = 0;
                for (size_t i = 0; i < oldVertexCount; ++i) {
                    MeshHelper::Vertex v = pSceneMesh_->vertices_[i];

                    auto index = uniqueVertices.find(v);
                    if (index == uniqueVertices.end()) {
                        uint32_t vertIndex = postTVertexCount;
                        postTVertexCount++;
                        uniqueVertices.insert(std::make_pair(v, vertIndex));
                        pSceneMesh_->vertices_[vertIndex] = v;
                        pSceneMesh_->indices_.push_back(vertIndex);
                    }
                    else {
                        pSceneMesh_->indices_.push_back(index->second);
                    }
                }
                pSceneMesh_->vertices_.resize(postTVertexCount);

                totalIndices_ += pSceneMesh_->indices_.size();
                totalVertices_ += pSceneMesh_->vertices_.size();
            }
        }
    }

    if (parent) {
        parent->children.push_back(scNode);
    }
    else {
        nodes.push_back(scNode);
    }
}

AnimatedGLTFObj::SceneNode* AnimatedGLTFObj::findNode(AnimatedGLTFObj::SceneNode* parent, uint32_t index)
{
    AnimatedGLTFObj::SceneNode* nodeFound = nullptr;
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

AnimatedGLTFObj::SceneNode* AnimatedGLTFObj::nodeFromIndex(uint32_t index)
{
    AnimatedGLTFObj::SceneNode* nodeFound = nullptr;
    for (auto& node : pNodes_)
    {
        nodeFound = findNode(node, index);
        if (nodeFound)
        {
            break;
        }
    }
    return nodeFound;
}

void AnimatedGLTFObj::loadSkins(tinygltf::Model& in, std::vector<Skin>& skins) {
    skins.resize(in.skins.size());

    for (size_t i = 0; i < in.skins.size(); i++) {
        tinygltf::Skin gltfSkin = in.skins[i];

        skins[i].name = gltfSkin.name;
        skins[i].skeletonRoot = nodeFromIndex(gltfSkin.skeleton);

        for (int jointInd : gltfSkin.joints) {
            SceneNode* node = nodeFromIndex(jointInd);
            if (node) {
                skins[i].joints.push_back(node);
            }
        }

        if (gltfSkin.inverseBindMatrices > -1) {
            const tinygltf::Accessor& accessor = in.accessors[gltfSkin.inverseBindMatrices];
            const tinygltf::BufferView& bufferView = in.bufferViews[accessor.bufferView];
            const tinygltf::Buffer& buffer = in.buffers[bufferView.buffer];
            skins[i].inverseBindMatrices.resize(accessor.count);
            size_t bufferSize = accessor.count * sizeof(glm::mat4);

            memcpy(skins[i].inverseBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], bufferSize);

            VkDeviceMemory bufferMemory;
            pDevHelper_->createBuffer(sizeof(glm::mat4) * skins[i].inverseBindMatrices.size(), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, skins[i].ssboBufferHandle, bufferMemory);

            vkMapMemory(pDevHelper_->getDevice(), bufferMemory, 0, bufferSize, 0, &skins[i].ssbo);
            memcpy(skins[i].ssbo, skins[i].inverseBindMatrices.data(), bufferSize);
        }
    }
}

void AnimatedGLTFObj::loadAnimations(tinygltf::Model& in, std::vector<Animation>& animations) {

    animations.resize(in.animations.size());

    for (size_t i = 0; i < in.animations.size(); i++)
    {
        tinygltf::Animation glTFAnimation = in.animations[i];
        animations[i].name = glTFAnimation.name;

        // Samplers
        animations[i].samplers.resize(glTFAnimation.samplers.size());
        for (size_t j = 0; j < glTFAnimation.samplers.size(); j++)
        {
            tinygltf::AnimationSampler glTFSampler = glTFAnimation.samplers[j];
            AnimationSampler& dstSampler = animations[i].samplers[j];
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
                for (auto input : animations[i].samplers[j].inputs)
                {
                    if (input < animations[i].start)
                    {
                        animations[i].start = input;
                    };
                    if (input > animations[i].end)
                    {
                        animations[i].end = input;
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
        animations[i].channels.resize(glTFAnimation.channels.size());
        for (size_t j = 0; j < glTFAnimation.channels.size(); j++)
        {
            tinygltf::AnimationChannel glTFChannel = glTFAnimation.channels[j];
            AnimationChannel& dstChannel = animations[i].channels[j];
            dstChannel.path = glTFChannel.target_path;
            dstChannel.samplerIndex = glTFChannel.sampler;
            dstChannel.node = nodeFromIndex(glTFChannel.target_node);
        }
    }
}

void AnimatedGLTFObj::loadGLTF() {
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

    if (loadedFile) {
        if (in.images.size() != 0) {
            loadImages(in, pSceneMesh_->images_);
            pSceneMesh_->textures_.resize(in.textures.size() + 3);
            loadMaterials(in, pSceneMesh_->mats_);
            loadTextures(in, pSceneMesh_->textures_);
        }

        const tinygltf::Scene& scene = in.scenes[0];
        for (size_t i = 0; i < scene.nodes.size(); i++) {
            const tinygltf::Node node = in.nodes[scene.nodes[i]];
            loadNode(in, node, scene.nodes[i], nullptr, pNodes_);
        }

        loadSkins(in, skins_);
        loadAnimations(in, animations_);

        for (auto node : pNodes_)
        {
            updateJoints(node);
        }
    }
    else {
        std::cout << "couldnt open gltf file" << std::endl;
    }

    pSceneMesh_->createVertexBuffer();

    pSceneMesh_->createIndexBuffer();
}

void AnimatedGLTFObj::loadMaterials(tinygltf::Model& in, std::vector<MeshHelper::Material>& mats) {
    uint32_t numImages = static_cast<uint32_t>(in.images.size());
    uint32_t dummyNormalIndex = numImages;
    uint32_t dummyMetallicRoughnessIndex = numImages + 1;
    uint32_t dummyAOIndex = numImages + 2;
    uint32_t dummyEmissionIndex = numImages + 3;
    mats.resize(in.materials.size());
    int count = 0;
    for (MeshHelper::Material& m : mats) {
        tinygltf::Material gltfMat = in.materials[count];
        if (gltfMat.values.find("baseColorFactor") != gltfMat.values.end()) {
            m.baseColor = glm::make_vec4(gltfMat.values["baseColorFactor"].ColorFactor().data());
        }
        if (gltfMat.values.find("baseColorTexture") != gltfMat.values.end()) {
            m.baseColorTexIndex = gltfMat.values["baseColorTexture"].TextureIndex();
            pSceneMesh_->images_[pSceneMesh_->textures_[m.baseColorTexIndex].textureIndex]->imageFormat_ = VK_FORMAT_R8G8B8A8_SRGB;

        }
        if (gltfMat.additionalValues.find("normalTexture") != gltfMat.additionalValues.end()) {
            m.normalTexIndex = gltfMat.additionalValues["normalTexture"].TextureIndex();
        }
        else {
            m.normalTexIndex = dummyNormalIndex;
        }
        if (gltfMat.values.find("metallicRoughnessTexture") != gltfMat.values.end()) {
            m.metallicRoughnessIndex = gltfMat.values["metallicRoughnessTexture"].TextureIndex();
        }
        else {
            m.metallicRoughnessIndex = dummyMetallicRoughnessIndex;
        }
        if (gltfMat.additionalValues.find("occlusionTexture") != gltfMat.additionalValues.end()) {
            m.aoIndex = gltfMat.additionalValues["occlusionTexture"].TextureIndex();
        }
        else {
            m.aoIndex = dummyAOIndex;
        }
        if (gltfMat.additionalValues.find("emissiveTexture") != gltfMat.additionalValues.end()) {
            m.emissionIndex = gltfMat.additionalValues["emissiveTexture"].TextureIndex();
            pSceneMesh_->images_[pSceneMesh_->textures_[m.emissionIndex].textureIndex]->imageFormat_ = VK_FORMAT_R8G8B8A8_SRGB;

        }
        else {
            m.emissionIndex = dummyEmissionIndex;
        }
        pSceneMesh_->images_[pSceneMesh_->textures_[m.normalTexIndex].textureIndex]->imageFormat_ = VK_FORMAT_R8G8B8A8_UNORM;
        pSceneMesh_->images_[pSceneMesh_->textures_[m.metallicRoughnessIndex].textureIndex]->imageFormat_ = VK_FORMAT_R8G8B8A8_UNORM;
        pSceneMesh_->images_[pSceneMesh_->textures_[m.aoIndex].textureIndex]->imageFormat_ = VK_FORMAT_R8G8B8A8_UNORM;

        m.alphaMode = gltfMat.alphaMode;
        m.alphaCutOff = (float)gltfMat.alphaCutoff;
        m.doubleSides = gltfMat.doubleSided;
        count++;
    }

    std::cout << std::endl << "loaded: " << mats.size() << " materials" << std::endl;
}

void AnimatedGLTFObj::createDescriptors(VkDescriptorSetLayout animDescSetLayout) {
    for (MeshHelper::Material& m : pSceneMesh_->mats_) {
        VkDescriptorSetAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.descriptorPool = pDevHelper_->getDescriptorPool();
        allocateInfo.descriptorSetCount = 1;
        const VkDescriptorSetLayout texSet = pDevHelper_->getTextureDescSetLayout();
        allocateInfo.pSetLayouts = &(texSet);

        VkResult res = vkAllocateDescriptorSets(pDevHelper_->getDevice(), &allocateInfo, &(m.descriptorSet));
        if (res != VK_SUCCESS) {
            std::_Xruntime_error("Failed to allocate descriptor sets!");
        }

        TextureHelper* t = pSceneMesh_->images_[pSceneMesh_->textures_[m.baseColorTexIndex].textureIndex];
        VkDescriptorImageInfo colorImageInfo{};
        colorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        colorImageInfo.imageView = t->textureImageView_;
        colorImageInfo.sampler = t->textureSampler_;

        TextureHelper* n = pSceneMesh_->images_[pSceneMesh_->textures_[m.normalTexIndex].textureIndex];
        VkDescriptorImageInfo normalImageInfo{};
        normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        normalImageInfo.imageView = n->textureImageView_;
        normalImageInfo.sampler = n->textureSampler_;

        TextureHelper* mR = pSceneMesh_->images_[pSceneMesh_->textures_[m.metallicRoughnessIndex].textureIndex];
        VkDescriptorImageInfo mrImageInfo{};
        mrImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        mrImageInfo.imageView = mR->textureImageView_;
        mrImageInfo.sampler = mR->textureSampler_;

        TextureHelper* ao = pSceneMesh_->images_[pSceneMesh_->textures_[m.aoIndex].textureIndex];
        VkDescriptorImageInfo aoImageInfo{};
        aoImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        aoImageInfo.imageView = ao->textureImageView_;
        aoImageInfo.sampler = ao->textureSampler_;

        TextureHelper* em = pSceneMesh_->images_[pSceneMesh_->textures_[m.emissionIndex].textureIndex];
        VkDescriptorImageInfo emissionImageInfo{};
        emissionImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        emissionImageInfo.imageView = em->textureImageView_;
        emissionImageInfo.sampler = em->textureSampler_;

        VkWriteDescriptorSet colorDescriptorWriteSet{};
        colorDescriptorWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        colorDescriptorWriteSet.dstSet = m.descriptorSet;
        colorDescriptorWriteSet.dstBinding = 0;
        colorDescriptorWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        colorDescriptorWriteSet.descriptorCount = 1;
        colorDescriptorWriteSet.pImageInfo = &colorImageInfo;

        VkWriteDescriptorSet normalDescriptorWriteSet{};
        normalDescriptorWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        normalDescriptorWriteSet.dstSet = m.descriptorSet;
        normalDescriptorWriteSet.dstBinding = 1;
        normalDescriptorWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        normalDescriptorWriteSet.descriptorCount = 1;
        normalDescriptorWriteSet.pImageInfo = &normalImageInfo;

        VkWriteDescriptorSet metallicRoughnessDescriptorWriteSet{};
        metallicRoughnessDescriptorWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        metallicRoughnessDescriptorWriteSet.dstSet = m.descriptorSet;
        metallicRoughnessDescriptorWriteSet.dstBinding = 2;
        metallicRoughnessDescriptorWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        metallicRoughnessDescriptorWriteSet.descriptorCount = 1;
        metallicRoughnessDescriptorWriteSet.pImageInfo = &mrImageInfo;

        VkWriteDescriptorSet aoDescriptorWriteSet{};
        aoDescriptorWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        aoDescriptorWriteSet.dstSet = m.descriptorSet;
        aoDescriptorWriteSet.dstBinding = 3;
        aoDescriptorWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        aoDescriptorWriteSet.descriptorCount = 1;
        aoDescriptorWriteSet.pImageInfo = &aoImageInfo;

        VkWriteDescriptorSet emDescriptorWriteSet{};
        emDescriptorWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        emDescriptorWriteSet.dstSet = m.descriptorSet;
        emDescriptorWriteSet.dstBinding = 4;
        emDescriptorWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        emDescriptorWriteSet.descriptorCount = 1;
        emDescriptorWriteSet.pImageInfo = &emissionImageInfo;

        std::vector<VkWriteDescriptorSet> descriptorWriteSets = { colorDescriptorWriteSet, normalDescriptorWriteSet, metallicRoughnessDescriptorWriteSet, aoDescriptorWriteSet, emDescriptorWriteSet };

        vkUpdateDescriptorSets(pDevHelper_->getDevice(), static_cast<uint32_t>(descriptorWriteSets.size()), descriptorWriteSets.data(), 0, nullptr);
    }

    for (auto& skin : skins_)
    {
        VkDescriptorSetAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.descriptorPool = pDevHelper_->getDescriptorPool();
        allocateInfo.descriptorSetCount = 1;
        const VkDescriptorSetLayout animatedSet = animDescSetLayout;
        allocateInfo.pSetLayouts = &(animatedSet);

        VkResult res = vkAllocateDescriptorSets(pDevHelper_->getDevice(), &allocateInfo, &(skin.descriptorSet));
        if (res != VK_SUCCESS) {
            std::_Xruntime_error("Failed to allocate descriptor sets!");
        }

        VkDescriptorBufferInfo descriptorBufferInfo{};
        descriptorBufferInfo.buffer = skin.ssboBufferHandle;
        descriptorBufferInfo.offset = 0;
        descriptorBufferInfo.range = sizeof(glm::mat4) * skin.inverseBindMatrices.size();

        VkWriteDescriptorSet animSkinWriteSet{};
        animSkinWriteSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        animSkinWriteSet.dstSet = skin.descriptorSet;
        animSkinWriteSet.dstBinding = 0;
        animSkinWriteSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        animSkinWriteSet.descriptorCount = 1;
        animSkinWriteSet.pBufferInfo = &descriptorBufferInfo;

        vkUpdateDescriptorSets(pDevHelper_->getDevice(), 1, &animSkinWriteSet, 0, nullptr);
    }
}

AnimatedGLTFObj::AnimatedGLTFObj(std::string gltfPath, DeviceHelper* deviceHelper) {
    gltfPath_ = gltfPath;
    pDevHelper_ = deviceHelper;
    pSceneMesh_ = new MeshHelper(deviceHelper);
    transparentCurrentBound = false;
}