#include "GLTFObject.h"

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

// render to color
void GLTFObj::drawIndexedOpaque(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) {
    for (auto& mat : opaqueDraws) {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &(mat.first->descriptorSet), 0, nullptr);
        for (auto& dC : mat.second) {
            glm::mat4 trueModelMatrix = localModelTransform * (*(dC->worldTransformMatrix));
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &(trueModelMatrix));
            MeshHelper::callIndexedDraw(commandBuffer, dC->indirectInfo);
        }
    }
}

void GLTFObj::drawIndexedTransparent(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) {
    for (auto& mat : transparentDraws) {
        Material* material = mat.first;
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &(material->descriptorSet), 0, nullptr);
        for (auto& dC : mat.second) {
            glm::mat4 trueModelMatrix = localModelTransform * (*(dC->worldTransformMatrix));
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &(trueModelMatrix));
            MeshHelper::callIndexedDraw(commandBuffer, dC->indirectInfo);
        }
    }
}

// render to cascades
void GLTFObj::drawShadow(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t cascadeIndex, SceneNode* node) {
    cascadeBlock.model = localModelTransform * node->worldTransform;
    vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(cascadeMVP), &cascadeBlock);
    for (auto& mesh : node->meshPrimitives) {
        MeshHelper::callIndexedDraw(commandBuffer, mesh->indirectInfo);
    }
    for (auto& child : node->children) {
        drawShadow(commandBuffer, pipelineLayout, cascadeIndex, child);
    }
}

void GLTFObj::renderShadow(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t cascadeIndex) {
    cascadeBlock.cascadeIndex = cascadeIndex;

    for (auto& node : pParentNodes) {
        drawShadow(commandBuffer, pipelineLayout, cascadeIndex, node);
    }
}

// render to depth buffer
void GLTFObj::drawDepth(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, SceneNode* node) {
    for (auto& mat : opaqueDraws) {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &(mat.first->descriptorSet), 0, nullptr);
        for (auto& dC : mat.second) {
            glm::mat4 trueModelMatrix = localModelTransform * (*(dC->worldTransformMatrix));
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &(trueModelMatrix));
            MeshHelper::callIndexedDraw(commandBuffer, dC->indirectInfo);
        }
    }
    for (auto& mat : transparentDraws) {
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &(mat.first->descriptorSet), 0, nullptr);
        pcBlock newBlock{ 1, mat.first->alphaCutOff };
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(glm::mat4), sizeof(pcBlock), &(newBlock));
        for (auto& dC : mat.second) {
            glm::mat4 trueModelMatrix = localModelTransform * (*(dC->worldTransformMatrix));
            vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &(trueModelMatrix));
            MeshHelper::callIndexedDraw(commandBuffer, dC->indirectInfo);
        }
    }
}

void GLTFObj::renderDepth(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) {
    for (auto& node : pParentNodes) {
        drawDepth(commandBuffer, pipelineLayout, node);
    }
}

void GLTFObj::loadNode(const tinygltf::Node& nodeIn, SceneNode* parent, uint32_t globalVertexOffset, uint32_t globalIndexOffset) {
    SMikkTSpaceContext mikktContext = { .m_pInterface = &MikkTInterface };

    SceneNode* scNode = new SceneNode{};
    scNode->worldTransform = glm::mat4(1.0f);
    scNode->parent = parent;

    if (nodeIn.translation.size() == 3) {
        scNode->worldTransform = glm::translate(scNode->worldTransform, glm::vec3(glm::make_vec3(nodeIn.translation.data())));
    }
    if (nodeIn.rotation.size() == 4) {
        glm::quat q = glm::make_quat(nodeIn.rotation.data());
        scNode->worldTransform *= glm::mat4(q);
    }
    if (nodeIn.scale.size() == 3) {
        scNode->worldTransform = glm::scale(scNode->worldTransform, glm::vec3(glm::make_vec3(nodeIn.scale.data())));
    }
    if (nodeIn.matrix.size() == 16) {
        scNode->worldTransform = glm::make_mat4x4(nodeIn.matrix.data());
    }

    if (nodeIn.children.size() > 0) {
        for (size_t i = 0; i < nodeIn.children.size(); i++) {
            loadNode(pInputModel_->nodes[nodeIn.children[i]], scNode, globalVertexOffset, globalIndexOffset);
        }
    }

    if (nodeIn.mesh > -1) {
        const tinygltf::Mesh mesh = pInputModel_->meshes[nodeIn.mesh];
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
            if (gltfPrims.attributes.find("POSITION") != gltfPrims.attributes.end()) {
                const tinygltf::Accessor& accessor = pInputModel_->accessors[gltfPrims.attributes.find("POSITION")->second];
                const tinygltf::BufferView& view = pInputModel_->bufferViews[accessor.bufferView];
                positionBuff = reinterpret_cast<const float*>(&(pInputModel_->buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
                currentNumVertices = static_cast<uint32_t>(accessor.count);
            }
            if (gltfPrims.attributes.find("NORMAL") != gltfPrims.attributes.end()) {
                const tinygltf::Accessor& accessor = pInputModel_->accessors[gltfPrims.attributes.find("NORMAL")->second];
                const tinygltf::BufferView& view = pInputModel_->bufferViews[accessor.bufferView];
                normalsBuff = reinterpret_cast<const float*>(&(pInputModel_->buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
            }
            if (gltfPrims.attributes.find("TEXCOORD_0") != gltfPrims.attributes.end()) {
                const tinygltf::Accessor& accessor = pInputModel_->accessors[gltfPrims.attributes.find("TEXCOORD_0")->second];
                const tinygltf::BufferView& view = pInputModel_->bufferViews[accessor.bufferView];
                uvBuff = reinterpret_cast<const float*>(&(pInputModel_->buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
            }
            if (gltfPrims.attributes.find("TANGENT") != gltfPrims.attributes.end()) {
                const tinygltf::Accessor& accessor = pInputModel_->accessors[gltfPrims.attributes.find("TANGENT")->second];
                const tinygltf::BufferView& view = pInputModel_->bufferViews[accessor.bufferView];
                tangentsBuff = reinterpret_cast<const float*>(&(pInputModel_->buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
            }

            for (size_t vert = 0; vert < currentNumVertices; vert++) {
                Vertex v;
                glm::vec2 uv = uvBuff ? glm::make_vec2(&uvBuff[vert * 2]) : glm::vec3(0.0f);
                glm::vec3 normal = glm::normalize(glm::vec3(normalsBuff ? glm::make_vec3(&normalsBuff[vert * 3]) : glm::vec3(0.0f)));

                v.pos = glm::vec4(glm::make_vec3(&positionBuff[vert * 3]), uv.x);
                v.normal = glm::vec4(normal, uv.y);
                v.tangent = tangentsBuff ? glm::make_vec4(&tangentsBuff[vert * 4]) : glm::vec4(0.0f);
                p->stagingVertices_.push_back(v);
            }

            // FOR INDEX
            const tinygltf::Accessor& accessor = pInputModel_->accessors[gltfPrims.indices];
            const tinygltf::BufferView& view = pInputModel_->bufferViews[accessor.bufferView];
            const tinygltf::Buffer& buffer = pInputModel_->buffers[view.buffer];

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
            p->indirectInfo.numIndices = currentNumIndices;
            p->indirectInfo.firstInstance = 0;
            p->indirectInfo.instanceCount = 1;
            p->indirectInfo.globalVertexOffset = globalVertexOffset;
            p->worldTransformMatrix = &scNode->worldTransform;
            p->materialIndex = gltfPrims.material;

            // TANGENT SPACE CREATION - ALSO REFERENCED OFF OF: https://github.com/Eearslya/glTFView.
            if (!tangentsBuff) {
                //UNPACK VERTICES
                std::vector<Vertex> unpacked(p->stagingIndices_.size());
                uint32_t newInd = 0;
                for (uint32_t index : p->stagingIndices_) {
                    unpacked[newInd] = p->stagingVertices_[static_cast<std::vector<Vertex, std::allocator<Vertex>>::size_type>(index) - firstVertex];
                    newInd++;
                }
                p->stagingVertices_ = std::move(unpacked);
                p->stagingIndices_.clear();

                // GEN TANGENT SPACE
                MikkTSpaceHelper::MikkTContext context{ p };
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
                        p->stagingIndices_.push_back(vertIndex);
                    }
                    else {
                        p->stagingIndices_.push_back(index->second);
                    }
                }
                p->stagingVertices_.resize(postTVertexCount);

                currentNumIndices = p->stagingIndices_.size();
                currentNumVertices = p->stagingVertices_.size();
            }

            totalIndices_ += currentNumIndices; 
            totalVertices_ += currentNumVertices;

            scNode->meshPrimitives.push_back(p);

            vertices_.insert(vertices_.end(), p->stagingVertices_.begin(), p->stagingVertices_.end());
            indices_.insert(indices_.end(), p->stagingIndices_.begin(), p->stagingIndices_.end());

            p->stagingIndices_.clear();
            p->stagingIndices_.shrink_to_fit();
            p->stagingVertices_.clear();
            p->stagingVertices_.shrink_to_fit();
        }
    }

    if (parent) {
        parent->children.push_back(scNode);
    }
    else {
        pParentNodes.push_back(scNode);
    }
}

void GLTFObj::loadGLTF(uint32_t globalVertexOffset, uint32_t globalIndexOffset) {
    pInputModel_ = new tinygltf::Model();
    tinygltf::TinyGLTF gltfContext;
    std::string error, warning;

    bool loadedFile = false;
    std::filesystem::path fPath = gltfPath_;
    if (fPath.extension() == ".glb") {
        loadedFile = gltfContext.LoadBinaryFromFile(pInputModel_, &error, &warning, gltfPath_);
    }
    else if (fPath.extension() == ".gltf") {
        loadedFile = gltfContext.LoadASCIIFromFile(pInputModel_, &error, &warning, gltfPath_);
    }

    if (loadedFile) {
        if (pInputModel_->images.size() != 0) {
            loadImages();
            textureIndices_.resize(pInputModel_->textures.size() + 3);
            loadMaterials();
            loadTextures();

            for (auto& image : images_) {
                image->load();
            }
        }

        const tinygltf::Scene& scene = pInputModel_->scenes[0];
        for (size_t i = 0; i < scene.nodes.size(); i++) {
            const tinygltf::Node node = pInputModel_->nodes[scene.nodes[i]];
            loadNode(node, nullptr, globalVertexOffset, globalIndexOffset);
        }
    }
    else {
        std::cout << "couldnt open gltf file" << std::endl;
    }

    delete pInputModel_;
}

// LOAD FUNCTIONS TEMPLATED FROM GLTFLOADING EXAMPLE ON GITHUB BY SASCHA WILLEMS
void GLTFObj::loadImages() {
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

    std::cout << std::endl << "loaded: " << (*(pInputModel_)).images.size() << " images" << std::endl;
}

void GLTFObj::loadTextures() {
    textureIndices_.resize((*(pInputModel_)).textures.size());
    uint32_t size = static_cast<uint32_t>(textureIndices_.size());
    for (size_t i = 0; i < (*(pInputModel_)).textures.size(); i++) {
        textureIndices_[i] = (*(pInputModel_)).textures[i].source;
    }
    textureIndices_.push_back(size);
    textureIndices_.push_back(size + 1);
    textureIndices_.push_back(size + 2);
    textureIndices_.push_back(size + 3);
}

void GLTFObj::loadMaterials() {
    uint32_t numImages = static_cast<uint32_t>((*(pInputModel_)).images.size());
    uint32_t dummyNormalIndex = numImages;
    uint32_t dummyMetallicRoughnessIndex = numImages + 1;
    uint32_t dummyAOIndex = numImages + 2;
    uint32_t dummyEmissionIndex = numImages + 3;
    mats_.resize((*(pInputModel_)).materials.size());
    for (int i = 0; i < mats_.size(); i++) {
        Material& m = mats_[i];
        tinygltf::Material gltfMat = (*(pInputModel_)).materials[i];
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
    }

    std::cout << std::endl << "loaded: " << mats_.size() << " materials" << std::endl;
}

void GLTFObj::createDescriptors() {
    for (Material& m : mats_) {
        VkDescriptorSetAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.descriptorPool = pDevHelper_->descPool_;
        allocateInfo.descriptorSetCount = 1;
        const VkDescriptorSetLayout texSet = pDevHelper_->texDescSetLayout_;
        allocateInfo.pSetLayouts = &(texSet);

        VkResult res = vkAllocateDescriptorSets(pDevHelper_->device_, &allocateInfo, &(m.descriptorSet));
        if (res != VK_SUCCESS) {
            std::cout << res;
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

        vkUpdateDescriptorSets(pDevHelper_->device_ , static_cast<uint32_t>(descriptorWriteSets.size()), descriptorWriteSets.data(), 0, nullptr);
    }
}

GLTFObj::GLTFObj(std::string gltfPath, DeviceHelper* deviceHelper, uint32_t globalVertexOffset, uint32_t globalIndexOffset) {
    gltfPath_ = gltfPath;
    pDevHelper_ = deviceHelper;
    this->globalFirstVertex = globalVertexOffset;
    this->globalFirstIndex = globalIndexOffset;

    loadGLTF(globalVertexOffset, globalIndexOffset);
}

// DELETION

void GLTFObj::recursiveDeleteNode(SceneNode* node) {
    for (auto& childNode : node->children) {
        recursiveDeleteNode(childNode);
    }

    for (auto& mesh : node->meshPrimitives) {
        delete mesh;
    }
    delete node;
}

GLTFObj::~GLTFObj() {
    for (auto& node : pParentNodes) {
        recursiveDeleteNode(node);
    }

    for (auto& texture : images_) {
        delete texture;
    }
}