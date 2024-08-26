#include "Camera.h"

void FPSCam::baseUpdate() {
    // to have a real camera matrix, you dont rotate/move the camera, but you move/rotate the world in the opposite direction of camera motion. Matrices are accumulated by shaders (VKGuide)
    glm::mat4 camTranslation = glm::translate(glm::mat4(1.0f), transform.position);

    glm::quat pitchRotation = glm::angleAxis(this->pitch_, glm::vec3{ -1.0f, 0.0f, 0.0f });
    glm::quat yawRotation = glm::angleAxis(this->yaw_, glm::vec3{ 0.0f, -1.0f, 0.0f });

    glm::mat4 rotationMatrix = (glm::toMat4(yawRotation) * glm::toMat4(pitchRotation));

    this->inverseViewMatrix = camTranslation * rotationMatrix;

    this->viewMatrix = glm::inverse(this->inverseViewMatrix);

    this->trueForward = inverseViewMatrix[2];
    this->right = inverseViewMatrix[0];
    this->up = inverseViewMatrix[1];
}

void FPSCam::update() {
    glm::vec3 localDisplacement{ 0.0f, 0.0f, 0.0f };
    if (Input::forwardKeyDown()) {
        localDisplacement -= trueForward;
    }

    if (Input::backwardKeyDown()) {
        localDisplacement += trueForward;
    }

    if (Input::rightKeyDown()) {
        localDisplacement += right;
    }

    if (Input::leftKeyDown()) {
        localDisplacement -= right;
    }

    if (Input::upKeyDown()) {
        localDisplacement += up;
    }

    if (Input::downKeyDown()) {
        localDisplacement -= up;
    }

    if (glm::length(localDisplacement) > 0) {
        setPosition(transform.position + (glm::normalize(localDisplacement) * this->moveSpeed_));
    }

    baseUpdate();

    setProjectionMatrix();
}

void FPSCam::alterUpdate(Transform playerTransform, float capHeight) {
    this->viewMatrix = glm::lookAt(transform.position, (playerTransform.position + glm::vec3(0.0f, capHeight, 0.0f)), glm::vec3(0.0f, 1.0f, 0.0f));
    this->inverseViewMatrix = glm::inverse(this->viewMatrix);

    this->trueForward = inverseViewMatrix[2];
    this->forward = glm::normalize(glm::vec3(trueForward.x, 0.0f, trueForward.z));
    this->right = inverseViewMatrix[0];
    this->up = inverseViewMatrix[1];
}

void FPSCam::physicsUpdate(Transform playerTransform, physx::PxScene* scene, physx::PxController* characterController, float capsuleHeight) {
    // physx raycast to find position
    glm::vec3 originVec = PxVec3toGlmVec3(characterController->getFootPosition()) + glm::vec3(0.0f, capsuleHeight + 1.0f, 0.0f);
    physx::PxVec3 origin = GlmVec3tpPxVec3(originVec);
    glm::vec3 desiredPos = glm::vec3(cos(pitch_) * cos(yaw_), sin(pitch_), cos(pitch_) * sin(yaw_));
    physx::PxVec3 unitDir = GlmVec3tpPxVec3(glm::normalize(desiredPos));
    physx::PxReal maxDistance = distanceToPlayer;
    physx::PxRaycastBuffer hitPayload;

    bool status = scene->raycast(origin, unitDir, maxDistance, hitPayload);

    if (status) {
        setPosition(PxVec3toGlmVec3(hitPayload.block.position + hitPayload.block.normal * 0.1f));
    }
    else {
        setPosition(PxVec3toGlmVec3(origin + (unitDir * distanceToPlayer)));
    }

    alterUpdate(playerTransform, capsuleHeight);
}

void FPSCam::processSDL(SDL_Event& event) {
    if (event.type == SDL_MOUSEMOTION) {
        this->yaw_ += ((float)event.motion.xrel / 30.f) * (PI / 180.0f);
        this->pitch_ += ((float)event.motion.yrel / 30.f) * (PI / 180.0f);
    }
}

void FPSCam::setProjectionMatrix() {
    this->projectionMatrix = glm::perspective(FOV, aspectRatio, nearPlane, farPlane);
    this->projectionMatrix[1][1] *= -1.0f;
}

glm::mat4 FPSCam::getProjectionMatrix() {
    return this->projectionMatrix;
}

glm::mat4 FPSCam::getViewMatrix() {
    return this->viewMatrix;
}

void FPSCam::setPosition(glm::vec3 pos) {
    transform.position = pos;
}

void FPSCam::setVelocity(glm::vec3 vel) {
    this->velocity_ = vel;
}

void FPSCam::setPitchYaw(float nPitch, float nYaw) {
    this->pitch_ = nPitch;
    this->yaw_ = nYaw;
}

void FPSCam::setNearPlane(float nearP) {
    this->nearPlane = nearP;
}

void FPSCam::setFarPlane(float farP) {
    this->farPlane = farP;
}

void FPSCam::setAspectRatio(float aspect) {
    this->aspectRatio = aspect;
}

void FPSCam::setFOV(float fov) {
    this->FOV = fov;
}