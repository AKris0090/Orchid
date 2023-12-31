#include "ModelHelper.h"
#include "VulkanRenderer.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
PRIVATE HELPER METHODS
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ModelHelper::createVertexBuffer() {
    VkDeviceSize bufferSize = sizeof(this->model.vertices[0]) * this->model.vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    this->vkR->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(this->vkR->device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, this->model.vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(this->vkR->device, stagingBufferMemory);

    this->vkR->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, this->vertexBuffer, this->vertexBufferMemory);
    this->vkR->copyBuffer(stagingBuffer, this->vertexBuffer, bufferSize);
    std::cout << "model vertex buffer" << std::endl;

    vkDestroyBuffer(this->vkR->device, stagingBuffer, nullptr);
    vkFreeMemory(this->vkR->device, stagingBufferMemory, nullptr);
}

void ModelHelper::createIndexBuffer() {
    VkDeviceSize bufferSize = sizeof(this->model.indices[0]) * this->model.indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    this->vkR->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(this->vkR->device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, this->model.indices.data(), (size_t)bufferSize);
    vkUnmapMemory(this->vkR->device, stagingBufferMemory);

    this->vkR->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
    this->vkR->copyBuffer(stagingBuffer, indexBuffer, bufferSize);
    std::cout << "model index buffer" << std::endl;

    vkDestroyBuffer(this->vkR->device, stagingBuffer, nullptr);
    vkFreeMemory(this->vkR->device, stagingBufferMemory, nullptr);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*
PUBLIC METHODS
*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ModelHelper::ModelHelper(std::string path, VulkanRenderer* vkR) {
    this->modPath = path;
    this->vkR = vkR;
    this->transform = glm::mat4 { 1.0f };
}

void ModelHelper::load() {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, modPath.c_str())) {
        throw std::runtime_error(warn + err);
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.texCoord = {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            vertex.color = { 1.0f, 1.0f, 1.0f };

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(model.vertices.size());
                model.vertices.push_back(vertex);
                model.totalVertices++;
            }

            model.indices.push_back(uniqueVertices[vertex]);
            model.totalIndices++;
        }
    }

    createVertexBuffer();

    createIndexBuffer();
}

void ModelHelper::destroy() {
    vkDestroyBuffer(this->vkR->device, vertexBuffer, nullptr);
    vkDestroyBuffer(this->vkR->device, indexBuffer, nullptr);

    vkFreeMemory(this->vkR->device, vertexBufferMemory, nullptr);
    vkFreeMemory(this->vkR->device, indexBufferMemory, nullptr);
}

void ModelHelper::render(VkCommandBuffer commandBuffer) {
    VkBuffer vertexBuffers[] = { vertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(this->model.indices.size()), 1, 0, 0, 0);
}