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

	int numModels_;
	int numAnimatedModels_;
	int numTextures_;
	std::vector<std::string> pStaticModelPaths_;
	std::vector<std::string> pAnimatedModelPaths_;
    std::vector<std::string> skyboxTexturePaths_;
	std::string skyboxModelPath_;

	void imGUIUpdate();
	static void check_vk_result(VkResult err);

	void setupImGUI();
	void startVulkan();
	void startSDL();

public:
	VulkanRenderer* pVkR_;
	SDL_Window* pWindow_;
	SDL_Renderer* pRenderer_;

	PlayerObject* player;

	std::vector<GameObject*> gameObjects = {};
	std::vector<AnimatedGameObject*> animatedObjects = {};

	bool mousemode_ = true;

	GraphicsManager(std::vector<std::string> staticModelPaths, std::vector<std::string> animatedModelPaths, std::string skyboxModelPath, std::vector<std::string> skyboxTexturePaths, float windowWidth, float windowHeight);

	void setup();
	void loopUpdate();
	void shutDown();
};