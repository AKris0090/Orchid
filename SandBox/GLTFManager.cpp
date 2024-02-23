#include "GLTFManager.h"

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES

void GLTFObj::drawIndexed(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, SceneNode* node) {
    if (node->mesh.primitives.size() > 0) {
        glm::mat4 nodeTransform = node->transform;
        SceneNode* curParent = node->parent;
        while (curParent) {
            nodeTransform = curParent->transform * nodeTransform;
            curParent = curParent->parent;
        }
        vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &nodeTransform);
        for (MeshHelper::PrimitiveObjIndices p : node->mesh.primitives) {
            if (p.numIndices > 0) {
                MeshHelper::Material mat = pSceneMesh->mats[p.materialIndex];
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mat.pipeline);
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &(mat.descriptorSet), 0, nullptr);
                vkCmdDrawIndexed(commandBuffer, p.numIndices, 1, p.firstIndex, 0, 0);
            }
        }
    }
    for (auto& child : node->children) {
        drawIndexed(commandBuffer, pipelineLayout, child);
    }
}

void GLTFObj::render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout) {
    VkBuffer vertexBuffers[] = { pSceneMesh->getVertexBuffer() };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, pSceneMesh->getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

    for (auto& node : pNodes) {
        drawIndexed(commandBuffer, pipelineLayout, node);
    }
}

// LOAD FUNCTIONS TEMPLATED FROM GLTFLOADING EXAMPLE ON GITHUB BY SASCHA WILLEMS
void GLTFObj::loadImages(tinygltf::Model& in, std::vector<TextureHelper*>& images) {
    for (size_t i = 0; i < in.images.size(); i++) {
        TextureHelper* tex = new TextureHelper(in, int(i), pDevHelper);
        tex->load();
        pSceneMesh->images.push_back(tex);
    }
    TextureHelper* dummyAO = new TextureHelper(in, -1, pDevHelper);
    TextureHelper* dummyMetallic = new TextureHelper(in, -2, pDevHelper);
    TextureHelper* dummyNormal = new TextureHelper(in, -3, pDevHelper);
    dummyAO->load();
    dummyMetallic->load();
    dummyNormal->load();
    pSceneMesh->images.push_back(dummyNormal);
    pSceneMesh->images.push_back(dummyMetallic);
    pSceneMesh->images.push_back(dummyAO);
}

void GLTFObj::loadTextures(tinygltf::Model& in, std::vector<TextureHelper::TextureIndexHolder>& textures) {
    pSceneMesh->textures.resize(in.textures.size());
    uint32_t size = static_cast<uint32_t>(textures.size());
    for (size_t i = 0; i < in.textures.size(); i++) {
        pSceneMesh->textures[i].textureIndex = in.textures[i].source;
    }
    pSceneMesh->textures.push_back(TextureHelper::TextureIndexHolder{ size });
    pSceneMesh->textures.push_back(TextureHelper::TextureIndexHolder{ size + 1 });
    pSceneMesh->textures.push_back(TextureHelper::TextureIndexHolder{ size + 2 });
}

void GLTFObj::loadNode(tinygltf::Model& in, const tinygltf::Node& nodeIn, SceneNode* parent, std::vector<SceneNode*>& nodes) {
    SceneNode* scNode = new SceneNode{};
    scNode->transform = glm::mat4(1.0f);
    scNode->parent = parent;

    if (nodeIn.translation.size() == 3) {
        scNode->transform = glm::translate(scNode->transform, glm::vec3(glm::make_vec3(nodeIn.translation.data())));
    }
    if (nodeIn.rotation.size() == 4) {
        glm::quat q = glm::make_quat(nodeIn.rotation.data());
        scNode->transform *= glm::mat4(q);
    }
    if (nodeIn.scale.size() == 3) {
        scNode->transform = glm::scale(scNode->transform, glm::vec3(glm::make_vec3(nodeIn.scale.data())));
    }
    if (nodeIn.matrix.size() == 16) {
        scNode->transform = glm::make_mat4x4(nodeIn.matrix.data());
    }

    if (nodeIn.children.size() > 0) {
        for (size_t i = 0; i < nodeIn.children.size(); i++) {
            loadNode(in, in.nodes[nodeIn.children[i]], scNode, nodes);
        }
    }

    if (nodeIn.mesh > -1) {
        const tinygltf::Mesh mesh = in.meshes[nodeIn.mesh];
        for (size_t i = 0; i < mesh.primitives.size(); i++) {
            const tinygltf::Primitive& gltfPrims = mesh.primitives[i];
            uint32_t firstIndex = static_cast<uint32_t>(pSceneMesh->indices.size());
            uint32_t vertexStart = static_cast<uint32_t>(pSceneMesh->vertices.size());
            uint32_t indexCount = 0;
            uint32_t numVertices = 0;

            // FOR VERTICES
            const float* positionBuff = nullptr;
            const float* normalsBuff = nullptr;
            const float* uvBuff = nullptr;
            const float* tangentsBuff = nullptr;
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

            for (size_t vert = 0; vert < numVertices; vert++) {
                MeshHelper::Vertex v;
                v.pos = glm::vec4(glm::make_vec3(&positionBuff[vert * 3]), 1.0f);
                v.normal = glm::normalize(glm::vec3(normalsBuff ? glm::make_vec3(&normalsBuff[vert * 3]) : glm::vec3(0.0f)));
                v.uv = uvBuff ? glm::make_vec2(&uvBuff[vert * 2]) : glm::vec3(0.0f);
                v.color = glm::vec3(1.0f);
                v.tangent = tangentsBuff ? glm::make_vec4(&tangentsBuff[vert * 4]) : glm::vec4(0.0f);
                pSceneMesh->vertices.push_back(v);
                _totalVertices++;
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
                    pSceneMesh->indices.push_back(buf[index] + vertexStart);
                }
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
                const uint16_t* buf = reinterpret_cast<const uint16_t*>(&buffer.data[accessor.byteOffset + view.byteOffset]);
                for (size_t index = 0; index < accessor.count; index++) {
                    pSceneMesh->indices.push_back(buf[index] + vertexStart);
                }
                break;
            }
            case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
                const uint8_t* buf = reinterpret_cast<const uint8_t*>(&buffer.data[accessor.byteOffset + view.byteOffset]);
                for (size_t index = 0; index < accessor.count; index++) {
                    pSceneMesh->indices.push_back(buf[index] + vertexStart);
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
            _totalIndices += indexCount;
            p.materialIndex = gltfPrims.material;
            scNode->mesh.primitives.push_back(p);
        }
    }

    if (parent) {
        parent->children.push_back(scNode);
    }
    else {
        nodes.push_back(scNode);
    }
}

void GLTFObj::loadGLTF() {
    tinygltf::Model in;
    tinygltf::TinyGLTF gltfContext;
    std::string error, warning;

    bool loadedFile = false;
    std::filesystem::path fPath = _gltfPath;
    if (fPath.extension() == ".glb") {
        loadedFile = gltfContext.LoadBinaryFromFile(&(in), &error, &warning, _gltfPath);
    }
    else if (fPath.extension() == ".gltf") {
        loadedFile = gltfContext.LoadASCIIFromFile(&(in), &error, &warning, _gltfPath);
    }

    if (loadedFile) {
        loadImages(in, pSceneMesh->images);
        pSceneMesh->textures.resize(in.textures.size() + 3);
        loadMaterials(in, pSceneMesh->mats);
        loadTextures(in, pSceneMesh->textures);

        const tinygltf::Scene& scene = in.scenes[0];
        for (size_t i = 0; i < scene.nodes.size(); i++) {
            const tinygltf::Node node = in.nodes[scene.nodes[i]];
            loadNode(in, node, nullptr, pNodes);
        }
    }
    else {
        std::cout << "couldnt open gltf file" << std::endl;
        std::_Xruntime_error("Failed to create the vertex buffer!");
    }

    pSceneMesh->createVertexBuffer();

    pSceneMesh->createIndexBuffer();
}

void GLTFObj::loadMaterials(tinygltf::Model& in, std::vector<MeshHelper::Material>& mats) {
    uint32_t numImages = static_cast<uint32_t>(in.images.size());
    std::cout << numImages << std::endl;
    uint32_t dummyNormalIndex = numImages;
    uint32_t dummyMetallicRoughnessIndex = numImages + 1;
    uint32_t dummyAOIndex = numImages + 2;
    mats.resize(in.materials.size());
    int count = 0;
    std::cout << mats.size() << std::endl;
    for (MeshHelper::Material& m : mats) {
        tinygltf::Material gltfMat = in.materials[count];
        if (gltfMat.values.find("baseColorFactor") != gltfMat.values.end()) {
            m.baseColor = glm::make_vec4(gltfMat.values["baseColorFactor"].ColorFactor().data());
        }
        if (gltfMat.values.find("baseColorTexture") != gltfMat.values.end()) {
            m.baseColorTexIndex = gltfMat.values["baseColorTexture"].TextureIndex();
            pSceneMesh->images[pSceneMesh->textures[m.baseColorTexIndex].textureIndex]->imageFormat = VK_FORMAT_R8G8B8A8_SRGB;

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
            pSceneMesh->images[pSceneMesh->textures[m.emissionIndex].textureIndex]->imageFormat = VK_FORMAT_R8G8B8A8_SRGB;

        }
        else {
            m.emissionIndex = dummyMetallicRoughnessIndex;
        }
        pSceneMesh->images[pSceneMesh->textures[m.normalTexIndex].textureIndex]->imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
        pSceneMesh->images[pSceneMesh->textures[m.metallicRoughnessIndex].textureIndex]->imageFormat = VK_FORMAT_R8G8B8A8_UNORM;
        pSceneMesh->images[pSceneMesh->textures[m.aoIndex].textureIndex]->imageFormat = VK_FORMAT_R8G8B8A8_UNORM;

        m.alphaMode = gltfMat.alphaMode;
        m.alphaCutOff = (float)gltfMat.alphaCutoff;
        m.doubleSides = gltfMat.doubleSided;
        count++;
    }
}

void GLTFObj::createDescriptors() {
    for (MeshHelper::Material& m : pSceneMesh->mats) {
        VkDescriptorSetAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocateInfo.descriptorPool = pDevHelper->getDescriptorPool();
        allocateInfo.descriptorSetCount = 1;
        const VkDescriptorSetLayout texSet = pDevHelper->getTextureDescSetLayout();
        allocateInfo.pSetLayouts = &(texSet);

        VkResult res = vkAllocateDescriptorSets(pDevHelper->getDevice(), &allocateInfo, &(m.descriptorSet));
        if (res != VK_SUCCESS) {
            std::_Xruntime_error("Failed to allocate descriptor sets!");
        }

        TextureHelper* t = pSceneMesh->images[pSceneMesh->textures[m.baseColorTexIndex].textureIndex];
        VkDescriptorImageInfo colorImageInfo{};
        colorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        colorImageInfo.imageView = t->textureImageView;
        colorImageInfo.sampler = t->textureSampler;

        TextureHelper* n = pSceneMesh->images[pSceneMesh->textures[m.normalTexIndex].textureIndex];
        VkDescriptorImageInfo normalImageInfo{};
        normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        normalImageInfo.imageView = n->textureImageView;
        normalImageInfo.sampler = n->textureSampler;

        TextureHelper* mR = pSceneMesh->images[pSceneMesh->textures[m.metallicRoughnessIndex].textureIndex];
        VkDescriptorImageInfo mrImageInfo{};
        mrImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        mrImageInfo.imageView = mR->textureImageView;
        mrImageInfo.sampler = mR->textureSampler;

        TextureHelper* ao = pSceneMesh->images[pSceneMesh->textures[m.aoIndex].textureIndex];
        VkDescriptorImageInfo aoImageInfo{};
        aoImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        aoImageInfo.imageView = ao->textureImageView;
        aoImageInfo.sampler = ao->textureSampler;

        TextureHelper* em = pSceneMesh->images[pSceneMesh->textures[m.emissionIndex].textureIndex];
        VkDescriptorImageInfo emissionImageInfo{};
        emissionImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        emissionImageInfo.imageView = em->textureImageView;
        emissionImageInfo.sampler = em->textureSampler;

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

        vkUpdateDescriptorSets(pDevHelper->getDevice() , static_cast<uint32_t>(descriptorWriteSets.size()), descriptorWriteSets.data(), 0, nullptr);
    }
}

GLTFObj::GLTFObj(std::string gltfPath, DeviceHelper* deviceHelper) {
    _gltfPath = gltfPath;
    pDevHelper = deviceHelper;
    pSceneMesh = new MeshHelper(deviceHelper);
}