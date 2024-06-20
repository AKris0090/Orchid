#include "PlayerObject.h"

PlayerObject::PlayerObject(physx::PxMaterial* material, physx::PxScene* pScene) {
	this->pScene_ = pScene;
	this->pMaterial_ = material;
	this->isWalking = false;
	this->turnDamping = 10.0f;
	this->setupPhysicsController();
}

void PlayerObject::setupPhysicsController() {
	manager = PxCreateControllerManager(*pScene_);
	desc.setToDefault();
	desc.radius = PLAYER_CAP_RADIUS;
	desc.height = PLAYER_CAP_HEIGHT;
	desc.position = physx::PxExtendedVec3(0.0, (double) ((cap_height / 2) + (cap_radius * 2)), 0.0);
	desc.material = pMaterial_;
	desc.stepOffset = 0.1f;
	desc.contactOffset = 0.001;
	desc.scaleCoeff = .99f;
	characterController = manager->createController(desc);

	physx::PxShape* shape;
	characterController->getActor()->getShapes(&shape, 1);
}

void PlayerObject::loopUpdate(FPSCam* camera) {
	glm::vec3 movementVector = glm::normalize(glm::vec3(camera->forward.x, 0.0f, camera->forward.z));
	glm::vec3 localDisplacement = glm::vec3(0.0f);

	if (Input::forwardKeyDown()) {
		localDisplacement -= movementVector * 0.5f;
	}

	if (Input::backwardKeyDown()) {
		localDisplacement += movementVector * 0.5f;
	}

	if (Input::rightKeyDown()) {
		localDisplacement += camera->right * 0.5f;
	}

	if (Input::leftKeyDown()) {
		localDisplacement -= camera->right * 0.5f;
	}

	if (glm::length(localDisplacement) != 0.0f) {
		isWalking = true;
		glm::vec3 normalizedDisplacement = glm::normalize(localDisplacement);
		localDisplacement = normalizedDisplacement * playerSpeed;
		playerGameObject->transform.rotation.y = Time::lerp(playerGameObject->transform.rotation.y, std::atan2(localDisplacement.x, localDisplacement.z), Time::getDeltaTime());
	}
	else {
		isWalking = false;
	}

	physx::PxFilterData filterData;
	filterData.word0 = 0;
	physx::PxControllerFilters data;
	data.mFilterData = &filterData;

	characterController->move(physx::PxVec3(localDisplacement.x, 0, localDisplacement.z), 0.001f, 1.0f / 60.0f, data);
	transform.position = PxVec3toGlmVec3(characterController->getFootPosition());
}