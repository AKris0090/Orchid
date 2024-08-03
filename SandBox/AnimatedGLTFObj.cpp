#include "AnimatedGLTFObj.h"

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

// MIKKTSPACE TANGENT FUNCTIONS, REFERENCED OFF OF: https://github.com/Eearslya/glTFView. SUCH AN AMAZING PERSON
struct MikkTContext {
    MeshHelper* mesh;
};

static int MikkTGetNumFaces(const SMikkTSpaceContext* context) {
    const auto data = reinterpret_cast<const MikkTContext*>(context->m_pUserData);
    return data->mesh->stagingVertices_.size() / 3;
}

static int MikkTGetNumVerticesOfFace(const SMikkTSpaceContext* context, const int face) {
    return 3;
}

static void MikkTGetPosition(const SMikkTSpaceContext* context, float fvPosOut[], const int face, const int vert) {
    const auto data = reinterpret_cast<const MikkTContext*>(context->m_pUserData);
    const glm::vec3 pos = data->mesh->stagingVertices_[face * 3 + vert].pos;
    fvPosOut[0] = pos.x;
    fvPosOut[1] = pos.y;
    fvPosOut[2] = pos.z;
}

static void MikkTGetNormal(const SMikkTSpaceContext* context, float fvNormOut[], const int face, const int vert) {
    const auto data = reinterpret_cast<const MikkTContext*>(context->m_pUserData);
    const glm::vec3 norm = data->mesh->stagingVertices_[face * 3 + vert].normal;
    fvNormOut[0] = norm.x;
    fvNormOut[1] = norm.y;
    fvNormOut[2] = norm.z;
}

static void MikkTGetTexCoord(const SMikkTSpaceContext* context, float fvTexcOut[], const int face, const int vert) {
    const auto data = reinterpret_cast<const MikkTContext*>(context->m_pUserData);
    glm::vec2 uv = glm::vec2(data->mesh->stagingVertices_[face * 3 + vert].pos.w, data->mesh->stagingVertices_[face * 3 + vert].normal.w);
    fvTexcOut[0] = uv.x;
    fvTexcOut[1] = 1.0 - uv.y;
}

static void MikkTSetTSpaceBasic(
    const SMikkTSpaceContext* context, const float fvTangent[], const float fSign, const int face, const int vert) {
    auto data = reinterpret_cast<MikkTContext*>(context->m_pUserData);

    data->mesh->stagingVertices_[face * 3 + vert].tangent = glm::vec4(glm::make_vec3(fvTangent), fSign);
}

static SMikkTSpaceInterface MikkTInterface = { .m_getNumFaces = MikkTGetNumFaces,
                                              .m_getNumVerticesOfFace = MikkTGetNumVerticesOfFace,
                                              .m_getPosition = MikkTGetPosition,
                                              .m_getNormal = MikkTGetNormal,
                                              .m_getTexCoord = MikkTGetTexCoord,
                                              .m_setTSpaceBasic = MikkTSetTSpaceBasic,
                                              .m_setTSpace = nullptr };

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void AnimatedGLTFObj::callIndexedDraw(VkCommandBuffer& commandBuffer, MeshHelper::indirectDrawInfo& indexedDrawInfo) {
    vkCmdDrawIndexed(commandBuffer, indexedDrawInfo.numIndices, indexedDrawInfo.instanceCount, indexedDrawInfo.firstIndex, indexedDrawInfo.globalVertexOffset, indexedDrawInfo.firstInstance);
}

void AnimatedGLTFObj::drawIndexedOpaque(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) {
    for (auto& mat : opaqueDraws) {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &(mat.first->descriptorSet), 0, nullptr);
        for (auto& draw : mat.second) {
            glm::mat4 trueModelMat = modelTransform * (*(draw->worldTransformMatrix));
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &(trueModelMat));
            callIndexedDraw(commandBuffer, draw->indirectInfo);
        }
    }
}

void AnimatedGLTFObj::drawIndexedOutline(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) {
    for (auto& mat : opaqueDraws) {
        for (auto& draw : mat.second) {
            glm::mat4 trueModelMat = modelTransform * (*(draw->worldTransformMatrix));
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &(trueModelMat));
            callIndexedDraw(commandBuffer, draw->indirectInfo);
        }
    }
}

void AnimatedGLTFObj::drawIndexedTransparent(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) {
    for (auto& mat : transparentDraws) {
        Material* material = mat.first;
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &(material->descriptorSet), 0, nullptr);
        //pcBlock newBlock{ 1, material->alphaCutOff };
        //vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(pcBlock), &(newBlock));
        for (auto& draw : mat.second) {
            glm::mat4 trueModelMat = modelTransform * (*(draw->worldTransformMatrix));
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &(trueModelMat));
            callIndexedDraw(commandBuffer, draw->indirectInfo);
        }
    }
}

void AnimatedGLTFObj::drawShadow(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t cascadeIndex, AnimSceneNode* node) {
    cascadeBlock.model = modelTransform * node->getAnimatedMatrix();
    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(cascadeMVP), &cascadeBlock);
    for (auto& mesh : node->meshPrimitives) {
        callIndexedDraw(commandBuffer, mesh->indirectInfo);
    }
    for (auto& child : node->children) {
        drawShadow(commandBuffer, pipelineLayout, cascadeIndex, child);
    }
}

void AnimatedGLTFObj::renderShadow(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t cascadeIndex) {
    cascadeBlock.cascadeIndex = cascadeIndex;

    for (auto& node : pParentNodes) {
        drawShadow(commandBuffer, pipelineLayout, cascadeIndex, node);
    }
}

void AnimatedGLTFObj::drawDepth(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, AnimSceneNode* node) {
    for (auto& mat : opaqueDraws) {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &(mat.first->descriptorSet), 0, nullptr);
        for (auto& dC : mat.second) {
            glm::mat4 trueModelMatrix = modelTransform * (*(dC->worldTransformMatrix));
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &(trueModelMatrix));
            callIndexedDraw(commandBuffer, dC->indirectInfo);
        }
    }
    for (auto& mat : transparentDraws) {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &(mat.first->descriptorSet), 0, nullptr);
        pcBlock newBlock{ 1, mat.first->alphaCutOff };
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(pcBlock), &(newBlock));
        for (auto& dC : mat.second) {
            glm::mat4 trueModelMatrix = modelTransform * (*(dC->worldTransformMatrix));
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &(trueModelMatrix));
            callIndexedDraw(commandBuffer, dC->indirectInfo);
        }
    }
}

void AnimatedGLTFObj::renderDepth(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) {
    for (auto& node : pParentNodes) {
        drawDepth(commandBuffer, pipelineLayout, node);
    }
}

glm::mat4 AnimSceneNode::getAnimatedMatrix() {
    return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale) * matrix;
}

// Also from Sascha Willems' gltfskinning example
glm::mat4 AnimatedGLTFObj::getNodeMatrix(AnimSceneNode* node)
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

// Also from Sascha Willems' gltfskinning example
void AnimatedGLTFObj::updateJoints(AnimSceneNode* node)
{
    if (node->skinIndex > -1)
    {
        // Update the joint matrices
        glm::mat4              inverseTransform = glm::inverse(getNodeMatrix(node));
        Skin                   skin = skins_[node->skinIndex];
        size_t                 numJoints = (uint32_t)skin.joints.size();
        for (size_t i = 0; i < numJoints; i++)
        {
            (*(skin.finalJointMatrices))[i] = inverseTransform * (getNodeMatrix(skin.joints[i]) * skin.inverseBindMatrices[i]);
        }
    }

    for (auto& child : node->children)
    {
        updateJoints(child);
    }
}

// LOAD FUNCTIONS TEMPLATED FROM GLTFLOADING EXAMPLE ON GITHUB BY SASCHA WILLEMS
void AnimatedGLTFObj::loadImages() {
    for (size_t i = 0; i < pInputModel_->images.size(); i++) {
        TextureHelper* tex = new TextureHelper(*(pInputModel_), int(i), pDevHelper_);
        images_.push_back(tex);
    }
    TextureHelper* dummyAO = new TextureHelper(*(pInputModel_), -1, pDevHelper_);
    TextureHelper* dummyMetallic = new TextureHelper(*(pInputModel_), -2, pDevHelper_);
    TextureHelper* dummyNormal = new TextureHelper(*(pInputModel_), -3, pDevHelper_);
    TextureHelper* dummyEmission = new TextureHelper(*(pInputModel_), -4, pDevHelper_);
    images_.push_back(dummyNormal);
    images_.push_back(dummyMetallic);
    images_.push_back(dummyAO);
    images_.push_back(dummyEmission);

    std::cout << std::endl << "loaded: " << pInputModel_->images.size() << "  images" << std::endl;
}

void AnimatedGLTFObj::loadTextures() {
    textureIndices_.resize(pInputModel_->textures.size());
    uint32_t size = static_cast<uint32_t>(images_.size() - 4);
    for (size_t i = 0; i < pInputModel_->textures.size(); i++) {
        textureIndices_[i] = pInputModel_->textures[i].source;
    }
    textureIndices_.push_back(size);
    textureIndices_.push_back(size + 1);
    textureIndices_.push_back(size + 2);
    textureIndices_.push_back(size + 3);
}

void AnimatedGLTFObj::loadNode(tinygltf::Model& in, const tinygltf::Node& nodeIn, uint32_t nodeIndex, AnimSceneNode* parent, std::vector<AnimSceneNode*>& nodes, uint32_t globalVertexOffset, uint32_t globalIndexOffset) {
    SMikkTSpaceContext mikktContext = { .m_pInterface = &MikkTInterface };

    AnimSceneNode* scNode = new AnimSceneNode{};
    scNode->matrix = glm::mat4(1.0f);
    scNode->skinIndex = nodeIn.skin;
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
            loadNode(in, in.nodes[nodeIn.children[i]], nodeIn.children[i], scNode, nodes, globalVertexOffset, globalIndexOffset);
        }
    }

    if (nodeIn.mesh > -1) {
        const tinygltf::Mesh mesh = in.meshes[nodeIn.mesh];
        for (size_t i = 0; i < mesh.primitives.size(); i++) {
            MeshHelper* p = new MeshHelper();

            const tinygltf::Primitive& gltfPrims = mesh.primitives[i];

            uint32_t firstIndex = totalIndices_;
            uint32_t firstVertex = totalVertices_;

            uint32_t currentNumIndices = 0;
            uint32_t currentNumVertices = 0;

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
                currentNumVertices = static_cast<uint32_t>(accessor.count);
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

            for (size_t vert = 0; vert < currentNumVertices; vert++) {
                glm::vec2 uv = uvBuff ? glm::make_vec2(&uvBuff[vert * 2]) : glm::vec3(0.0f);
                glm::vec3 normal = glm::normalize(glm::vec3(normalsBuff ? glm::make_vec3(&normalsBuff[vert * 3]) : glm::vec3(0.0f)));

                Vertex v;
                v.pos = glm::vec4(glm::make_vec3(&positionBuff[vert * 3]), uv.x);
                v.normal = glm::vec4(normal, uv.y);
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
                p->stagingVertices_.push_back(v);
            }

            // FOR INDEX
            const tinygltf::Accessor& accessor = in.accessors[gltfPrims.indices];
            const tinygltf::BufferView& view = in.bufferViews[accessor.bufferView];
            const tinygltf::Buffer& buffer = in.buffers[view.buffer];

            currentNumIndices += static_cast<uint32_t>(accessor.count);

            switch (accessor.componentType) {
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
                const uint32_t* buf = reinterpret_cast<const uint32_t*>(&buffer.data[accessor.byteOffset + view.byteOffset]);
                for (size_t index = 0; index < accessor.count; index++) {
                    p->stagingIndices_.push_back(buf[index] + firstVertex);
                }
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                const uint16_t* buf = reinterpret_cast<const uint16_t*>(&buffer.data[accessor.byteOffset + view.byteOffset]);
                for (size_t index = 0; index < accessor.count; index++) {
                    p->stagingIndices_.push_back(buf[index] + firstVertex);
                }
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                const uint8_t* buf = reinterpret_cast<const uint8_t*>(&buffer.data[accessor.byteOffset + view.byteOffset]);
                for (size_t index = 0; index < accessor.count; index++) {
                    p->stagingIndices_.push_back(buf[index] + firstVertex);
                }
                break;
            }
            default:
                std::cout << "index component type not supported" << std::endl;
                std::_Xruntime_error("index component type not supported");
            }

            p->indirectInfo.firstIndex = firstIndex + globalIndexOffset;
            p->indirectInfo.firstInstance = 0;
            p->indirectInfo.instanceCount = 1;
            p->indirectInfo.globalVertexOffset = globalVertexOffset;
            p->worldTransformMatrix = &scNode->matrix;
            p->materialIndex = gltfPrims.material;

            // TANGENT SPACE CREATION - ALSO REFERENCED OFF OF: https://github.com/Eearslya/glTFView.
            if (!tangentsBuff) {
                //UNPACK VERTICES
                std::vector<Vertex> unpacked(p->stagingIndices_.size());
                uint32_t newInd = 0;
                for (uint32_t index : p->stagingIndices_) {
                    unpacked[newInd] = p->stagingVertices_[index - firstVertex];
                    newInd++;
                }
                p->stagingVertices_ = std::move(unpacked);
                p->stagingIndices_.clear();

                // GEN TANGENT SPACE
                MikkTContext context{ p };
                mikktContext.m_pUserData = &context;
                genTangSpaceDefault(&mikktContext);

                //WELD VERTICES
                p->stagingIndices_.clear();
                p->stagingIndices_.reserve(p->stagingVertices_.size());
                std::unordered_map<Vertex, uint32_t> uniqueVertices;

                size_t oldVertexCount = p->stagingVertices_.size();
                uint32_t postTVertexCount = 0;
                for (size_t i = 0; i < oldVertexCount; ++i) {
                    Vertex v = p->stagingVertices_[i];

                    auto index = uniqueVertices.find(v);
                    if (index == uniqueVertices.end()) {
                        uint32_t vertIndex = postTVertexCount;
                        postTVertexCount++;
                        uniqueVertices.insert(std::make_pair(v, vertIndex));
                        p->stagingVertices_[vertIndex] = v;
                        p->stagingIndices_.push_back(vertIndex + firstVertex);
                    }
                    else {
                        p->stagingIndices_.push_back(index->second + firstVertex);
                    }
                }
                p->stagingVertices_.resize(postTVertexCount);

                currentNumIndices = p->stagingIndices_.size();
                currentNumVertices = p->stagingVertices_.size();
            }

            p->indirectInfo.numIndices = currentNumIndices;

            totalIndices_ += currentNumIndices;
            totalVertices_ += currentNumVertices;

            scNode->meshPrimitives.push_back(p);
        }
    }

    if (parent) {
        parent->children.push_back(scNode);
    }
    else {
        pParentNodes.push_back(scNode);
    }
}

AnimSceneNode* AnimatedGLTFObj::findNode(AnimSceneNode* parent, uint32_t index)
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

AnimSceneNode* AnimatedGLTFObj::nodeFromIndex(uint32_t index)
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

void AnimatedGLTFObj::loadSkins() {
    skins_.resize(pInputModel_->skins.size());

    for (size_t i = 0; i < pInputModel_->skins.size(); i++) {
        tinygltf::Skin gltfSkin = pInputModel_->skins[i];

        skins_[i].name = gltfSkin.name;
        skins_[i].skeletonRoot = nodeFromIndex(gltfSkin.skeleton);

        for (int jointInd : gltfSkin.joints) {
            AnimSceneNode* node = nodeFromIndex(jointInd);
            if (node) {
                skins_[i].joints.push_back(node);
            }
        }

        if (gltfSkin.inverseBindMatrices > -1) {
            const tinygltf::Accessor& accessor = pInputModel_->accessors[gltfSkin.inverseBindMatrices];
            const tinygltf::BufferView& bufferView = pInputModel_->bufferViews[accessor.bufferView];
            const tinygltf::Buffer& buffer = pInputModel_->buffers[bufferView.buffer];
            skins_[i].inverseBindMatrices.resize(accessor.count);
            size_t bufferSize = accessor.count * sizeof(glm::mat4);

            memcpy(skins_[i].inverseBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], bufferSize);
        }
    }
}

void AnimatedGLTFObj::loadGLTF(uint32_t globalVertexOffset, uint32_t globalIndexOffset) {
    tinygltf::Model in;
    tinygltf::TinyGLTF gltfContext;
    std::string error, warning;

    pInputModel_ = &in;

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
            loadImages();
            textureIndices_.resize(pInputModel_->images.size() + 3);
            loadTextures();
            loadMaterials();

            for (auto& image : images_) {
                image->load();
            }
        }

        const tinygltf::Scene& scene = in.scenes[0];
        for (size_t i = 0; i < scene.nodes.size(); i++) {
            const tinygltf::Node node = in.nodes[scene.nodes[i]];
            loadNode(in, node, scene.nodes[i], nullptr, pParentNodes, globalVertexOffset, globalIndexOffset);
        }

        loadSkins();
        walkAnim.loadAnimation(gltfPath_, pParentNodes);
        //loadAnimations(in, animations_);

        for (auto& skin : skins_) {
            skin.finalJointMatrices = new std::vector<glm::mat4>();
            skin.finalJointMatrices->resize(skin.inverseBindMatrices.size());
        }

        for (auto node : pParentNodes)
        {
            updateJoints(node);
        }
    }
    else {
        std::cout << "couldnt open gltf file" << std::endl;
    }
}

void AnimatedGLTFObj::loadMaterials() {
    uint32_t numImages = static_cast<uint32_t>(pInputModel_->textures.size());
    uint32_t dummyNormalIndex = numImages;
    uint32_t dummyMetallicRoughnessIndex = numImages + 1;
    uint32_t dummyAOIndex = numImages + 2;
    uint32_t dummyEmissionIndex = numImages + 3;
    mats_.resize(pInputModel_->materials.size());
    int count = 0;
    for (Material& m : mats_) {
        tinygltf::Material gltfMat = pInputModel_->materials[count];
        if (gltfMat.values.find("baseColorFactor") != gltfMat.values.end()) {
            m.baseColor = glm::make_vec4(gltfMat.values["baseColorFactor"].ColorFactor().data());
        }
        if (gltfMat.values.find("baseColorTexture") != gltfMat.values.end()) {
            m.baseColorTexIndex = gltfMat.values["baseColorTexture"].TextureIndex();
            images_[textureIndices_[m.baseColorTexIndex]]->imageFormat_ = VK_FORMAT_R8G8B8A8_SRGB;

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
            images_[textureIndices_[m.emissionIndex]]->imageFormat_ = VK_FORMAT_R8G8B8A8_SRGB;

        }
        else {
            m.emissionIndex = dummyEmissionIndex;
        }
        images_[textureIndices_[m.normalTexIndex]]->imageFormat_ = VK_FORMAT_R8G8B8A8_UNORM;
        images_[textureIndices_[m.metallicRoughnessIndex]]->imageFormat_ = VK_FORMAT_R8G8B8A8_UNORM;
        images_[textureIndices_[m.aoIndex]]->imageFormat_ = VK_FORMAT_R8G8B8A8_UNORM;

        m.alphaMode = gltfMat.alphaMode;
        m.alphaCutOff = (float)gltfMat.alphaCutoff;
        m.doubleSides = gltfMat.doubleSided;
        count++;
    }

    std::cout << std::endl << "loaded: " << mats_.size() << " materials" << std::endl;
}

void AnimatedGLTFObj::createDescriptors() {
    for (Material& m : mats_) {
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

        TextureHelper* t = images_[textureIndices_[m.baseColorTexIndex]];
        VkDescriptorImageInfo colorImageInfo{};
        colorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        colorImageInfo.imageView = t->textureImageView_;
        colorImageInfo.sampler = t->textureSampler_;

        TextureHelper* n = images_[textureIndices_[m.normalTexIndex]];
        VkDescriptorImageInfo normalImageInfo{};
        normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        normalImageInfo.imageView = n->textureImageView_;
        normalImageInfo.sampler = n->textureSampler_;

        TextureHelper* mR = images_[textureIndices_[m.metallicRoughnessIndex]];
        VkDescriptorImageInfo mrImageInfo{};
        mrImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        mrImageInfo.imageView = mR->textureImageView_;
        mrImageInfo.sampler = mR->textureSampler_;

        TextureHelper* ao = images_[textureIndices_[m.aoIndex]];
        VkDescriptorImageInfo aoImageInfo{};
        aoImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        aoImageInfo.imageView = ao->textureImageView_;
        aoImageInfo.sampler = ao->textureSampler_;

        TextureHelper* em = images_[textureIndices_[m.emissionIndex]];
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
}

void AnimatedGLTFObj::recursiveVertexAdd(std::vector<Vertex>* vertices, AnimSceneNode* parentNode) {
    for (auto& mesh : parentNode->meshPrimitives) {
        for (int i = 0; i < mesh->stagingVertices_.size(); i++) {
            vertices->push_back(mesh->stagingVertices_[i]);
        }
    }

    for (auto& node : parentNode->children) {
        recursiveVertexAdd(vertices, node);
    }
}

void AnimatedGLTFObj::recursiveIndexAdd(std::vector<uint32_t>* indices, AnimSceneNode* parentNode) {
    for (auto& mesh : parentNode->meshPrimitives) {
        for (int i = 0; i < mesh->stagingIndices_.size(); i++) {
            indices->push_back(mesh->stagingIndices_[i]);
        }
    }

    for (auto& node : parentNode->children) {
        recursiveIndexAdd(indices, node);
    }
}

void AnimatedGLTFObj::addVertices(std::vector<Vertex>* vertices) {
    for (auto& node : pParentNodes) {
        recursiveVertexAdd(vertices, node);
    }
}

void AnimatedGLTFObj::addIndices(std::vector<uint32_t>* indices) {
    for (auto& node : pParentNodes) {
        recursiveIndexAdd(indices, node);
    }
}

void AnimatedGLTFObj::recursiveRemove(AnimSceneNode* parentNode) {
    for (auto& mesh : parentNode->meshPrimitives) {
        mesh->stagingVertices_.clear();
        mesh->stagingVertices_.shrink_to_fit();
        mesh->stagingIndices_.clear();
        mesh->stagingIndices_.shrink_to_fit();
    }

    for (auto& node : parentNode->children) {
        recursiveRemove(node);
    }
}

void AnimatedGLTFObj::remove() {
    for (auto& node : pParentNodes) {
        recursiveRemove(node);
    }
}

AnimatedGLTFObj::AnimatedGLTFObj(std::string gltfPath, DeviceHelper* deviceHelper, uint32_t globalVertexOffset, uint32_t globalIndexOffset) {
    gltfPath_ = gltfPath;
    pDevHelper_ = deviceHelper;
    this->globalFirstVertex = globalVertexOffset;
    this->globalFirstIndex = globalIndexOffset;
}