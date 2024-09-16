#pragma once

#include "GameObject.h"
#include "AnimatedGameObject.h"

#define BLOOM_LEVELS 4
static constexpr int NUMBER_OF_DOWNSAMPLED_IMAGES = BLOOM_LEVELS - 1;

class BloomHelper {
private:
	struct Attachment {
		VkImageView imageView;
		VkFormat format;
		VkExtent2D imageExtent;
		VkAttachmentLoadOp loadOp;
		VkAttachmentStoreOp storeOp;
		VkImageLayout initialImageLayout;
		VkImageLayout finalImageLayout;
		VkImageLayout subImageLayout;
	};

	struct FrameBufferHolder {
		VkFramebuffer frameBuffer;
		VkExtent2D extent;
	};

	DeviceHelper* pDevHelper_;

	VkImage* emissionImage;
	VkImageView* emissionImageView_;
	VkFormat emissionImageFormat;
	VkExtent2D emissionImageExtent;

	VkShaderModule createShaderModule(const std::vector<char>& binary);
	std::vector<char> readFile(const std::string& filename);

	void createImageViews();
	void createAttachments();
	void createRenderPasses();
	void createFramebuffersDown();
	void createFramebuffersUp();

	void createDescriptors();
	void createBloomPipelines();
public:
	struct pushConstantBloom {
		glm::vec4 resolutionRadius;
	};

	std::vector<Attachment> mipChainAttachmentsDown;
	std::vector<Attachment> mipChainAttachmentsUp;

	std::vector<VkImageView> imageViewMipChain;

	VkRenderPass bloomRenderPassUp;
	VkRenderPass bloomRenderPassDown;

	VkSampler bloomSampler;

	std::vector<FrameBufferHolder> frameBuffersUp;
	std::vector<FrameBufferHolder> frameBuffersDown;

	VkDescriptorSetLayout bloomSetLayout;
	std::vector<VkDescriptorSet> bloomSets;

	VkPipelineLayout bloomPipelineLayout;
	VkPipeline bloomPipelineUp;
	VkPipeline bloomPipelineDown;

	void setupBloom(VkImage* emissionImage, VkImageView* emissionImageView, VkFormat emissionImageFormat, VkExtent2D emissionImageExtent);
	void startBloomRenderPass(VkCommandBuffer& commandBuffer, VkRenderPass* renderPass, FrameBufferHolder* framebuffer);
	void setViewport(VkCommandBuffer& commandBuffer, VkExtent2D& extent);
	BloomHelper(DeviceHelper* devH);
};