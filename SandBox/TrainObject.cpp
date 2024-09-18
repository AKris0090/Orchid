#include "TrainObject.h"

TrainObject::TrainObject(glm::vec3 startPos, float enterTime, float exitTime, float openTime, float waitTime) {
	this->transform.position = startPos;
	this->startPos = startPos;
	this->currentState = TRAINSTATE::IDLEWAITING;
	this->needsEnter = false;
	this->enterDuration = enterTime;
	this->exitDuration = exitTime;
	this->doorOpenDuration = openTime;
	this->doorWaitDuration = waitTime;
}

void TrainObject::updatePosition() {
	trainBodyObject->transform.position = transform.position;
	trainLeftDoorObject->transform.position = transform.position + leftDoorTransform.position;
	trainRightDoorObject->transform.position = transform.position + rightDoorTransform.position;
	trainBodyObject->renderTarget->localModelTransform = trainBodyObject->transform.to_matrix();
	trainLeftDoorObject->renderTarget->localModelTransform = trainLeftDoorObject->transform.to_matrix();
	trainRightDoorObject->renderTarget->localModelTransform = trainRightDoorObject->transform.to_matrix();
}

void TrainObject::loopUpdate() {
	if (Input::leftMouseDown() && currentState == TRAINSTATE::IDLEWAITING) {
		currentState = TRAINSTATE::ISENTERING;
		startTime = Time::getCurrentTime();
		transitionTimer = 0.0f;
	}

	currentTime += std::chrono::milliseconds(static_cast<int>(Time::getDeltaTime()));

	switch (currentState) {	
	case ISENTERING: {
		transitionTimer += Time::getDeltaTime() * 1000 / enterDuration;

		float blend = Time::smoothStep(0, 1, transitionTimer);

		this->transform.position = Time::weightLerp(startPos, glm::vec3(0.0f, 0.0f, 0.0f), blend);

		if (Time::getCurrentTime() > (startTime + std::chrono::milliseconds(static_cast<int>(enterDuration)))) {
			startTime = Time::getCurrentTime();
			transitionTimer = 0.0f;
			currentState = TRAINSTATE::DONEENTERING;
		}
	}
		break;

	case DONEENTERING: {
		// open doors
		transitionTimer += Time::getDeltaTime() * 1000 / doorOpenDuration;

		float blend = Time::smoothStep(0, 1, transitionTimer);

		this->leftDoorTransform.position = Time::weightLerp(glm::vec3(0.0f), glm::vec3(0.445f, 0.0f, 0.0f), blend);
		this->rightDoorTransform.position = Time::weightLerp(glm::vec3(0.0f), glm::vec3(-0.445f, 0.0f, 0.0f), blend);

		if (Time::getCurrentTime() > (startTime + std::chrono::milliseconds(static_cast<int>(doorOpenDuration)))) {
			startTime = Time::getCurrentTime();
			currentState = TRAINSTATE::DOORSOPEN;
		}
	}
		break;
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

		this->leftDoorTransform.position = Time::weightLerp(glm::vec3(0.445f, 0.0f, 0.0f), glm::vec3(0.0f), blend);
		this->rightDoorTransform.position = Time::weightLerp(glm::vec3(-0.445f, 0.0f, 0.0f), glm::vec3(0.0f), blend);

		if (Time::getCurrentTime() > (startTime + std::chrono::milliseconds(static_cast<int>(doorOpenDuration)))) {
			startTime = Time::getCurrentTime();
			transitionTimer = 0.0f;
			currentState = TRAINSTATE::LEAVING;
		}
	}
			
		break;
	case LEAVING: {
		transitionTimer += Time::getDeltaTime() * 1000 / exitDuration;

		float blend = Time::smoothStep(0, 1, transitionTimer);

		this->transform.position = Time::weightLerp(glm::vec3(0.0f, 0.0f, 0.0f), -startPos, blend);

		if (Time::getCurrentTime() > (startTime + std::chrono::milliseconds(static_cast<int>(exitDuration)))) {
			transform.position = startPos;
			currentState = TRAINSTATE::IDLEWAITING;
		}
	}
		break;

	default:

		break;
	}

	updatePosition();
}