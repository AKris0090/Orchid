#pragma once

#include "Input.h"
#include "GameObject.h"
#include "Time.h"

enum TRAINSTATE {
	IDLEWAITING,
	ISENTERING,
	DONEENTERING,
	DOORSOPEN,
	DOORSWAITING,
	LEAVING
};

class TrainObject: public GameObject {
public:
	TRAINSTATE currentState;
	int doorOpenDirection;
	glm::vec3 startPos;
	glm::vec3 endPos;
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

	TrainObject() {};
	void setup(Transform startTransform, glm::vec3 endPos, float enterTime, float exitTime, float openTime, float waitTime, int openDirection);
	void updatePosition();
	void transitionState();
	void scriptUpdate() override;
};