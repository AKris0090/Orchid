#pragma once

#include "GLTFObject.h"
#include "VulkanUtils.h"
#include "stb_image.h"

class Skybox {
private:
	std::string modPath_;
	std::vector<std::string> texPaths_;
	DeviceHelper* pDevHelper_;
	uint32_t mipLevels_;
	VkFormat imageFormat_;
	stbi_uc* pixels[6];

	VkBuffer stagingBuffer_;
	VkDeviceMemory stagingBufferMemory_;
	VkImage skyBoxImage_;
	VkDeviceMemory skyBoxImageMemory_;

	void createDescriptorSet();
	void createSkyBoxImage();
	void createSkyBoxImageView();
	void createSkyBoxImageSampler();
	void createSkyBoxDescriptorSetLayout();
	void loadSkyBox(uint32_t globalVertexOffset, uint32_t globalIndexOffset);

public:
	GLTFObj* pSkyBoxModel_;

	VkDescriptorSetLayout skyBoxDescriptorSetLayout_;
	VkDescriptorSet skyBoxDescriptorSet_;
	VkImageView skyBoxImageView_;
	VkSampler skyBoxImageSampler_;
	VkDescriptorPool skyBoxDescriptorPool_;
	VulkanPipelineBuilder* skyBoxPipeline_;

	void drawSkyBoxIndexed(VkCommandBuffer& commandBuffer);
	Skybox(std::string modPath, std::vector<std::string> texPaths, DeviceHelper* devHelper, uint32_t globalVertexOffset, uint32_t globalIndexOffset);
	~Skybox();
};