#pragma once

#include "MeshHelper.h"
#include "Camera.h"
#include "Input.h"
#include "GameObject.h"
#include "AnimatedGameObject.h"
#include "Time.h"

enum TRAINSTATE {
	IDLEWAITING,
	ISENTERING,
	DONEENTERING,
	DOORSOPEN,
	DOORSWAITING,
	LEAVING
};

class TrainObject {
public:
	GameObject* trainBodyObject;
	GameObject* trainLeftDoorObject;
	GameObject* trainRightDoorObject;

	TRAINSTATE currentState;

	glm::vec3 startPos;
	Transform transform;
	Transform leftDoorTransform;
	Transform rightDoorTransform;
	bool needsEnter;
	float transitionTimer;

	std::chrono::time_point<std::chrono::system_clock> startTime;
	std::chrono::time_point<std::chrono::system_clock> currentTime;
	float enterDuration;
	float exitDuration;
	float doorOpenDuration;
	float doorWaitDuration;

	TrainObject(glm::vec3 startPos, float enterTime, float exitTime, float openTime, float waitTime);
	void updatePosition();
	void loopUpdate();
};