#include "PlayerObject.h"

PlayerObject::PlayerObject(physx::PxMaterial* material, physx::PxScene* pScene) {
	this->pScene_ = pScene;
	this->pMaterial_ = material;
	this->currentState = PLAYERSTATE::IDLE;
	this->turnSpeed = 10.0f;
	this->setupPhysicsController();
}

void PlayerObject::transitionState(PLAYERSTATE newState) {
	previousState = currentState;
	playerGameObject->previousAnimation = playerGameObject->activeAnimation;
	switch(newState) {
	case PLAYERSTATE::IDLE:
		currentState = PLAYERSTATE::IDLE;
		playerGameObject->activeAnimation = &(playerGameObject->renderTarget->idleAnim);
		break;
	case PLAYERSTATE::WALKING:
		currentState = PLAYERSTATE::WALKING;
		currentSpeed = playerWalkSpeed;
		playerGameObject->activeAnimation = &(playerGameObject->renderTarget->walkAnim);
		break;
	case PLAYERSTATE::RUNNING:
		currentState = PLAYERSTATE::RUNNING;
		currentSpeed = playerRunSpeed;
		playerGameObject->activeAnimation = &(playerGameObject->renderTarget->runAnim);
		break;
	default:
		break;
	}
	playerGameObject->needsSmooth = true;
}

void PlayerObject::setupPhysicsController() {
	manager = PxCreateControllerManager(*pScene_);
	desc.setToDefault();
	desc.radius = PLAYER_CAP_RADIUS;
	desc.height = PLAYER_CAP_HEIGHT;
	desc.position = physx::PxExtendedVec3(0.0, (double) ((cap_height / 2) + (cap_radius * 2)), 0.0);
	desc.material = pMaterial_;
	desc.stepOffset = 0.5f;
	desc.contactOffset = 0.001;
	desc.scaleCoeff = .99f;
	characterController = manager->createController(desc);

	physx::PxShape* shape;
	characterController->getActor()->getShapes(&shape, 1);
}

void PlayerObject::loopUpdate(FPSCam* camera) {
	if (camera->isAttatched) {
		glm::vec3 localDisplacement = glm::vec3(0.0f);

		if (Input::forwardKeyDown()) {
			localDisplacement -= camera->forward;
		}

		if (Input::backwardKeyDown()) {
			localDisplacement += camera->forward;
		}

		if (Input::rightKeyDown()) {
			localDisplacement += camera->right;
		}

		if (Input::leftKeyDown()) {
			localDisplacement -= camera->right;
		}

		if (glm::length(localDisplacement) != 0.0f) {
			if (Input::shiftKeyDown()) {
				if (currentState == PLAYERSTATE::WALKING || currentState == PLAYERSTATE::IDLE) {
					transitionState(RUNNING);
				}
			}
			else {
				if (currentState == PLAYERSTATE::RUNNING || currentState == PLAYERSTATE::IDLE) {
					transitionState(WALKING);
				}
			}
			localDisplacement = glm::normalize(localDisplacement);
			localDisplacement *= currentSpeed;
			float theta = std::atan2(localDisplacement.x, localDisplacement.z);
			if (theta - playerGameObject->transform.rotation.y > PI) {
				theta -= 2.0f * PI;
			}
			else if (theta - playerGameObject->transform.rotation.y < -PI) {
				theta += 2.0f * PI;
			}
			playerGameObject->transform.rotation.y = Time::lerp(playerGameObject->transform.rotation.y, theta, Time::getDeltaTime() * turnSpeed);
		}
		else {
			if (currentState != PLAYERSTATE::IDLE) {
				transitionState(IDLE);
			}
			currentState = PLAYERSTATE::IDLE;
			currentSpeed = 0.0f;
		}

		physx::PxFilterData filterData;
		filterData.word0 = 0;
		physx::PxControllerFilters data;
		data.mFilterData = &filterData;

		characterController->move(physx::PxVec3(localDisplacement.x, -transform.position.y, localDisplacement.z), 0.001f, Time::getDeltaTime(), data);
		playerGameObject->transform.position = transform.position = PxVec3toGlmVec3(characterController->getFootPosition());
		playerGameObject->setTransform(playerGameObject->transform.to_matrix());
	}
}