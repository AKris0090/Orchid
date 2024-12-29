#include "TrainObject.h"

void TrainObject::setup(Transform newT, glm::vec3 endPos, float enterTime, float exitTime, float openTime, float waitTime, int openDirection) {
	this->transform.position = newT.position;
	for (const auto& t : renderTargetTransforms) {
		t->rotation = newT.rotation;
	}
	this->startPos = newT.position;
	this->currentState = TRAINSTATE::IDLEWAITING;
	this->needsEnter = false;
	this->endPos = endPos;
	this->enterDuration = enterTime;
	this->exitDuration = exitTime;
	this->doorOpenDuration = openTime;
	this->doorWaitDuration = waitTime;
	this->doorOpenDirection = openDirection;
}

void TrainObject::updatePosition() {
	renderTargetTransforms[0]->position = transform.position;
	renderTargetTransforms[1]->position = transform.position + leftDoorTransform.position;
	renderTargetTransforms[2]->position = transform.position + rightDoorTransform.position;
}

void TrainObject::transitionState() {
	switch (currentState) {
	case ISENTERING: {
		transitionTimer += Time::getDeltaTime() * 1000 / enterDuration;

		float blend = Time::smoothStep(0, 1, transitionTimer);

		this->transform.position = Time::weightLerp(startPos, endPos, blend);

		if (Time::getCurrentTime() > (startTime + std::chrono::milliseconds(static_cast<int>(enterDuration)))) {
			startTime = Time::getCurrentTime();
			transitionTimer = 0.0f;
			currentState = TRAINSTATE::DONEENTERING;
		}
		break;
	}

	case DONEENTERING: {
		// open doors
		transitionTimer += Time::getDeltaTime() * 1000 / doorOpenDuration;

		float blend = Time::smoothStep(0, 1, transitionTimer);

		this->leftDoorTransform.position = Time::weightLerp(glm::vec3(0.0f), glm::vec3(this->doorOpenDirection * 0.445f, 0.0f, 0.0f), blend);
		this->rightDoorTransform.position = Time::weightLerp(glm::vec3(0.0f), glm::vec3(this->doorOpenDirection  * -0.445f, 0.0f, 0.0f), blend);

		if (Time::getCurrentTime() > (startTime + std::chrono::milliseconds(static_cast<int>(doorOpenDuration)))) {
			startTime = Time::getCurrentTime();
			currentState = TRAINSTATE::DOORSOPEN;
		}

		break;
	}
	case DOORSOPEN:
		// wait
		if (Time::getCurrentTime() > (startTime + std::chrono::milliseconds(static_cast<int>(doorWaitDuration)))) {
			startTime = Time::getCurrentTime();
			transitionTimer = 0.0f;
			currentState = TRAINSTATE::DOORSWAITING;
		}
		break;
	case DOORSWAITING: {
		// close door
		transitionTimer += Time::getDeltaTime() * 1000 / doorOpenDuration;

		float blend = Time::smoothStep(0, 1, transitionTimer);

		this->leftDoorTransform.position = Time::weightLerp(glm::vec3(this->doorOpenDirection * 0.445f, 0.0f, 0.0f), glm::vec3(0.0f), blend);
		this->rightDoorTransform.position = Time::weightLerp(glm::vec3(this->doorOpenDirection * -0.445f, 0.0f, 0.0f), glm::vec3(0.0f), blend);

		if (Time::getCurrentTime() > (startTime + std::chrono::milliseconds(static_cast<int>(doorOpenDuration)))) {
			startTime = Time::getCurrentTime();
			transitionTimer = 0.0f;
			currentState = TRAINSTATE::LEAVING;
		}
		break;
	}
	case LEAVING: {
		transitionTimer += Time::getDeltaTime() * 1000 / exitDuration;

		float blend = Time::smoothStep(0, 1, transitionTimer);

		this->transform.position = Time::weightLerp(endPos, glm::vec3(-startPos.x, 0.0f, startPos.z), blend);

		if (Time::getCurrentTime() > (startTime + std::chrono::milliseconds(static_cast<int>(exitDuration)))) {
			transform.position = startPos;
			currentState = TRAINSTATE::IDLEWAITING;
		}
		break;
	}
	default:
		break;
	}
}

void TrainObject::scriptUpdate() {
	if (Input::leftMouseDown() && currentState == TRAINSTATE::IDLEWAITING) {
		currentState = TRAINSTATE::ISENTERING;
		startTime = Time::getCurrentTime();
		transitionTimer = 0.0f;
	}

	currentTime += std::chrono::milliseconds(static_cast<int>(Time::getDeltaTime()));
	transitionState();

	updatePosition();
}