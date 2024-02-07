#pragma once

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>

#include "VulkanRenderer.h"

class GraphicsManager {
private:
	int numModels;
	int numTextures;
	std::string* model_paths;
    std::string* texture_paths;

	const int MAX_FRAMES_IN_FLIGHT = 1;

	static void check_vk_result(VkResult err);

	void setupImGUI();
	void startVulkan();
	void startSDL();
public:
	// Create vulkan rendering pipeline
	VulkanRenderer* vkR;
	SDL_Window* window;
	SDL_Renderer* renderer;

	bool mousemode = true;

	GraphicsManager(std::string modelStr[], int numModels, int numTextures);
	void setup();
	void loopUpdate();
	void shutDown();
};