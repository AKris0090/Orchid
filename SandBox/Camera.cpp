#include "Camera.h"

void FPSCam::update() {
	glm::mat4 camrotation = getRotationMatrix();
	transform.position += glm::vec3(camrotation * glm::vec4(velocity_ * 0.5f, 0.0f));
}

void FPSCam::processSDL(SDL_Event& event) {
    if (event.type == SDL_MOUSEMOTION) {
        this->yaw_ += (float)event.motion.xrel / 300.f;
        this->pitch_ -= (float)event.motion.yrel / 300.f;
    }
}

glm::mat4 FPSCam::getViewMatrix() {
    // to have a real camera matrix, you dont rotate/move the camera, but you move/rotate the world in the opposite direction of camera motion. Matrices are accumulated by shaders (VKGuide)
    glm::mat4 camTranslation = glm::translate(glm::mat4(1.0f), transform.position);
    glm::mat4 camRotation = this->getRotationMatrix();

    this->viewMatrix = glm::inverse(camTranslation * camRotation);

    return this->viewMatrix;
}

glm::mat4 FPSCam::getRotationMatrix() {
    glm::quat pitchRotation = glm::angleAxis(this->pitch_, glm::vec3{ 1.0f, 0.0f, 0.0f });
    // inverse left-right, since left is negative x
    glm::quat yawRotation = glm::angleAxis(this->yaw_, glm::vec3{ 0.0f, -1.0f, 0.0f });

    return (glm::toMat4(yawRotation) * glm::toMat4(pitchRotation));
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