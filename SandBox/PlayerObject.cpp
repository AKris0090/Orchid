#include "PlayerObject.h"

void PlayerObject::setup(physx::PxMaterial* material, physx::PxScene* pScene, FPSCam* cam) {
	this->pScene_ = pScene;
	this->pMaterial_ = material;
	this->currentState = PLAYERSTATE::IDLE;
	this->turnSpeed = 10.0f;
	this->setupPhysicsController();
	this->camera = cam;
}

void PlayerObject::transitionState(PLAYERSTATE newState) {
	previousState = currentState;
	previousAnimation = activeAnimation;
	switch(newState) {
	case PLAYERSTATE::IDLE:
		currentState = PLAYERSTATE::IDLE;
		activeAnimation = &(idleAnim);
		break;
	case PLAYERSTATE::WALKING:
		currentState = PLAYERSTATE::WALKING;
		currentSpeed = playerWalkSpeed;
		activeAnimation = &(walkAnim);
		break;
	case PLAYERSTATE::RUNNING:
		currentState = PLAYERSTATE::RUNNING;
		currentSpeed = playerRunSpeed;
		activeAnimation = &(runAnim);
		break;
	case PLAYERSTATE::GUNAIM:
		currentState = PLAYERSTATE::GUNAIM;
		currentSpeed = playerWalkSpeed;
		activeAnimation = &(gunAnim);
		break;
	default:
		break;
	}
	needsSmooth = true;
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

void PlayerObject::scriptUpdate() {
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
			localDisplacement = glm::normalize(localDisplacement) * currentSpeed;

			if (Input::rightMouseDown()) {
				if (currentState != PLAYERSTATE::GUNAIM) {
					transitionState(GUNAIM);
				}
			}
			else {
				if (Input::shiftKeyDown()) {
					if (currentState != PLAYERSTATE::RUNNING) {
						transitionState(RUNNING);
					}
				}
				else {
					if (currentState != PLAYERSTATE::WALKING) {
						transitionState(WALKING);
					}
				}
			}
		}
		else {
			if (currentState != PLAYERSTATE::IDLE && !Input::rightMouseDown()) {
				transitionState(IDLE);
				currentSpeed = 0.0f;
			}
			else if (Input::rightMouseDown() && currentState != PLAYERSTATE::GUNAIM) {
				transitionState(PLAYERSTATE::GUNAIM);
			}
		}

		if (currentState == PLAYERSTATE::GUNAIM) {
			float theta = std::atan2(-camera->forward.x, -camera->forward.z);
			transform.rotation.y = Time::lerp(transform.rotation.y, theta, Time::getDeltaTime() * turnSpeed);
		}
		else {
			float theta = std::atan2(localDisplacement.x, localDisplacement.z);
			if (theta - transform.rotation.y > PI) {
				theta -= 2.0f * PI;
			}
			else {
				theta += 2.0f * PI;
			}
			transform.rotation.y = Time::lerp(transform.rotation.y, theta, Time::getDeltaTime() * turnSpeed);
		}

		physx::PxFilterData filterData;
		filterData.word0 = 0;
		physx::PxControllerFilters data;
		data.mFilterData = &filterData;

		characterController->move(physx::PxVec3(localDisplacement.x, -transform.position.y, localDisplacement.z), 0.001f, Time::getDeltaTime(), data);
		transform.position = PxVec3toGlmVec3(characterController->getFootPosition());
	}
}