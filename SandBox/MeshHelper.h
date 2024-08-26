#pragma once

#include "TextureHelper.h"
#include "mikktspace.h"

struct Material {
	glm::vec4 baseColor = glm::vec4(1.0f);
	uint32_t baseColorTexIndex;
	uint32_t normalTexIndex;
	uint32_t metallicRoughnessIndex;
	uint32_t aoIndex;
	uint32_t emissionIndex;
	std::string alphaMode = "OPAQUE";
	float alphaCutOff;
	bool doubleSides = false;
	VkDescriptorSet descriptorSet;
};

struct Vertex {
	glm::vec4 pos; // uv_x0 stored in w
	glm::vec4 normal; // uv_y0 stored in w
	glm::vec4 tangent;
	glm::vec4 jointIndices;
	glm::vec4 jointWeights;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(Vertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 1> getPositionAttributeDescription() {
		std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions{};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);
		return attributeDescriptions;
	}

	static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, normal);
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(Vertex, tangent);
		return attributeDescriptions;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getDepthAttributeDescription() {
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, normal);
		return attributeDescriptions;
	}

	bool operator==(const Vertex& other) const {
		return pos == other.pos && normal == other.normal && tangent == other.tangent;
	}

	Vertex(glm::vec2 pos, glm::vec2 tex) {
		this->pos.x = pos.x;
		this->pos.y = pos.y;
		this->pos.z = tex.x;
		this->pos.w = tex.y;
		this->normal = glm::vec4(0.0f);
		this->tangent = glm::vec4(0.0f);
		this->jointIndices = glm::vec4(0.0f);
		this->jointWeights = glm::vec4(0.0f);
	}

	Vertex() {
		this->pos = glm::vec4(0.0f);
		this->normal = glm::vec4(0.0f);
		this->tangent = glm::vec4(0.0f);
		this->jointIndices = glm::vec4(0.0f);
		this->jointWeights = glm::vec4(0.0f);
	};
};

class MeshHelper {
public:
	uint32_t materialIndex;
	uint32_t skinIndex;	
	glm::mat4 worldTransformMatrix;
	std::vector<uint32_t> stagingIndices_ = {};
	std::vector<Vertex> stagingVertices_ = {};

	MeshHelper() {
		this->materialIndex = 0;
		this->skinIndex = -1;
		this->worldTransformMatrix = glm::mat4(1.0f);
		this->indirectInfo = {};
	};

	struct indirectDrawInfo {
		uint32_t numIndices;
		uint32_t instanceCount;
		uint32_t firstIndex;
		int32_t globalVertexOffset;
		uint32_t firstInstance;
	} indirectInfo;

	static void callIndexedDraw(VkCommandBuffer& commandBuffer, MeshHelper::indirectDrawInfo& indexedDrawInfo) {
		vkCmdDrawIndexed(commandBuffer, indexedDrawInfo.numIndices, indexedDrawInfo.instanceCount, indexedDrawInfo.firstIndex, indexedDrawInfo.globalVertexOffset, indexedDrawInfo.firstInstance);
	}
};

namespace std {
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return (hash<glm::vec4>()(vertex.pos) ^ hash<glm::vec4>()(vertex.normal) ^ hash<glm::vec4>()(vertex.tangent));
		}
	};
}

// MIKKTSPACE TANGENT FUNCTIONS, REFERENCED OFF OF: https://github.com/Eearslya/glTFView. SUCH AN AMAZING PERSON
class MikkTSpaceHelper {
public:

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

		data->mesh->stagingVertices_[static_cast<std::vector<Vertex, std::allocator<Vertex>>::size_type>(face) * 3 + vert].tangent = glm::vec4(glm::make_vec3(fvTangent), fSign);
	}
};

static SMikkTSpaceInterface MikkTInterface = { .m_getNumFaces = MikkTSpaceHelper::MikkTGetNumFaces,
											   .m_getNumVerticesOfFace = MikkTSpaceHelper::MikkTGetNumVerticesOfFace,
											   .m_getPosition = MikkTSpaceHelper::MikkTGetPosition,
											   .m_getNormal = MikkTSpaceHelper::MikkTGetNormal,
											   .m_getTexCoord = MikkTSpaceHelper::MikkTGetTexCoord,
											   .m_setTSpaceBasic = MikkTSpaceHelper::MikkTSetTSpaceBasic,
											   .m_setTSpace = nullptr };
