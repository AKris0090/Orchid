#pragma once

#include "GLTFManager.h"
#include "stb_image.h"

class Skybox {
private:
	std::string modPath_;
	std::vector<std::string> texPaths_;
	VkDevice device_;
	DeviceHelper* pDevHelper_;
	uint32_t mipLevels_;
	VkFormat imageFormat_;
	stbi_uc* pixels[6];

	VkBuffer stagingBuffer_;
	VkDeviceMemory stagingBufferMemory_;

	VkImage skyBoxImage_;
	VkImageView skyBoxImageView_;
	VkSampler skyBoxImageSampler_;

	VkDeviceMemory skyBoxImageMemory_;

	void transitionImageLayout(VkCommandBuffer cmdBuff, VkImageSubresourceRange subresourceRange, VkImageLayout oldLayout, VkImageLayout newLayout);
	void copyBufferToImage(VkCommandBuffer cmdBuff, VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
	void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
	void createDescriptorSet();

	void createSkyBoxImage();
	void createSkyBoxImageView();
	void createSkyBoxImageSampler();

	void createSkyBoxDescriptorSetLayout();

	void skyBoxLoad();
public:
	GLTFObj* pSkyBoxModel_;
	VkPipelineLayout skyBoxPipelineLayout_;
	VkPipeline skyboxPipeline_;
	VkDescriptorSetLayout skyBoxDescriptorSetLayout_;
	VkDescriptorSet skyBoxDescriptorSet_;

	Skybox() {};
	Skybox(std::string modPath, std::vector<std::string> stringPaths, DeviceHelper* devHelper);

	VkImageView getImageView() { return this->skyBoxImageView_; };

	void loadSkyBox();
};