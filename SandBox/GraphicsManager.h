#pragma once

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_vulkan.h>
#include "VulkanRenderer.h"
#include "PlayerObject.h"
#include "TrainObject.h"
#include "Time.h"

class GraphicsManager {
private:
	float windowWidth;
	float windowHeight;
	uint32_t frameCount = 0;

	void imGUIUpdate();
	static void check_vk_result(VkResult err);

	void setupImGUI();
	void startVulkan(std::vector<std::string>& staticModelPaths, std::vector<std::string>& animatedModelPaths, std::string& skyboxModelPath, std::vector<std::string>& skyboxTexturePaths);
	void startSDL();

public:
	VulkanRenderer vkR_;
	SDL_Window* pWindow_;
	SDL_Renderer* pRenderer_;

	std::vector<GameObject*> staticGameObjects;
	std::vector<AnimatedGameObject*> animatedGameObjects;

	void updateModelMatrices(bool ignoreDynamic);

	GraphicsManager() {}
	~GraphicsManager() {
		//shutDown();
	}
	GraphicsManager(float windowWidth, float windowHeight);

	void setup(std::vector<std::string>& staticModelPaths, std::vector<std::string>& animatedModelPaths, std::string& skyboxModelPath, std::vector<std::string>& skyboxTexturePaths);
	void loopUpdate();
	void shutDown();
};