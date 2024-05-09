#pragma once

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>
#include "VulkanRenderer.h"

class GraphicsManager {
private:
	int MAX_FRAMES_IN_FLIGHT = 3;

	int numModels_;
	int numAnimated;
	int numTextures_;
	std::string* pModelPaths_;
    std::vector<std::string> skyboxTexturePaths_;
	std::string skyboxModelPath_;

	static void check_vk_result(VkResult err);
	void imGUIUpdate();

	void setupImGUI();
	void startVulkan();
	void startSDL();
public:
	VulkanRenderer* pVkR_;
	SDL_Window* pWindow_;
	SDL_Renderer* pRenderer_;
	VkDescriptorSet m_Dset;

	std::vector<GameObject*> gameObjects = {};
	std::vector<AnimatedGameObject*> animatedObjects = {};

	bool mousemode_ = true;

	GraphicsManager(std::string modelStr[], std::string skyBoxModelPath, std::vector<std::string> skyBoxPaths, int numModels, int numTextures);

	void setup();
	void loopUpdate();
	void shutDown();
};