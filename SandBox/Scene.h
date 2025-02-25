#pragma once

#include "GraphicsManager.h"
#include "PhysicsManager.h"

class Scene {
private:
public:
	GraphicsManager graphicsManager;
	PhysicsManager physicsManager;
	PlayerObject* player;

	Scene() {};
	void setupScene();
	void loopUpdate() {
		for (auto& g : this->graphicsManager.staticGameObjects) {
			g->loopUpdate();
		}
		for (auto& aG : this->graphicsManager.animatedGameObjects) {
			if (aG->updateAnim) {
				aG->updateAnimation(graphicsManager.vkR_.inverseBindMatrices, Time::getDeltaTime());
			}
			aG->loopUpdate();
		}
		graphicsManager.vkR_.updateBindMatrices();
	}
};