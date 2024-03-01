#include "Camera.h"

void FPSCam::update() {
	glm::mat4 camrotation = getRotationMatrix();
	position_ += glm::vec3(camrotation * glm::vec4(velocity_ * 0.5f, 0.0f));
}

void FPSCam::processSDL(SDL_Event& event) {
    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_w) { this->velocity_.z = -moveSpeed_; }
        if (event.key.keysym.sym == SDLK_s) { this->velocity_.z = moveSpeed_; }
        if (event.key.keysym.sym == SDLK_a) { this->velocity_.x = -moveSpeed_; }
        if (event.key.keysym.sym == SDLK_d) { this->velocity_.x = moveSpeed_; }
        if (event.key.keysym.sym == SDLK_e) { this->velocity_.y = moveSpeed_; }
        if (event.key.keysym.sym == SDLK_q) { this->velocity_.y = -moveSpeed_; }
    }

    if (event.type == SDL_KEYUP) {
        if (event.key.keysym.sym == SDLK_w) { this->velocity_.z = 0.0f; }
        if (event.key.keysym.sym == SDLK_s) { this->velocity_.z = 0.0f; }
        if (event.key.keysym.sym == SDLK_a) { this->velocity_.x = 0.0f; }
        if (event.key.keysym.sym == SDLK_d) { this->velocity_.x = 0.0f; }
        if (event.key.keysym.sym == SDLK_e) { this->velocity_.y = 0.0f; }
        if (event.key.keysym.sym == SDLK_q) { this->velocity_.y = 0.0f; }
    }

    if (event.type == SDL_MOUSEMOTION) {
        this->yaw_ += (float)event.motion.xrel / 300.f;
        this->pitch_ -= (float)event.motion.yrel / 300.f;
    }
}

glm::mat4 FPSCam::getViewMatrix() {
    // to have a real camera matrix, you dont rotate/move the camera, but you move/rotate the world in the opposite direction of camera motion. Matrices are accumulated by shaders (VKGuide)
    glm::mat4 camTranslation = glm::translate(glm::mat4(1.0f), this->position_);
    glm::mat4 camRotation = this->getRotationMatrix();

    //this->viewPos_ = glm::vec4(this->position_, 0.0f) * glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f);
    this->viewPos_ = glm::vec4(this->position_, 0.0f);
    //glm::mat4 viewMatrix = glm::inverse(camTranslation * camRotation);
    //viewMatrix = glm::transpose(glm::inverse(viewMatrix));
    //viewPos_ = glm::vec4(viewMatrix[2][0], viewMatrix[2][1], viewMatrix[2][2], 1.0f);

    return glm::inverse(camTranslation * camRotation);
}

glm::mat4 FPSCam::getRotationMatrix() {
    glm::quat pitchRotation = glm::angleAxis(this->pitch_, glm::vec3{ 1.0f, 0.0f, 0.0f });
    // inverse left-right, since left is negative x
    glm::quat yawRotation = glm::angleAxis(this->yaw_, glm::vec3{ 0.0f, -1.0f, 0.0f });

    return (glm::toMat4(yawRotation) * glm::toMat4(pitchRotation));
}

void FPSCam::setPosition(glm::vec3 pos) {
    this->position_ = pos;
}

void FPSCam::setVelocity(glm::vec3 vel) {
    this->velocity_ = vel;
}

void FPSCam::setPitchYaw(float nPitch, float nYaw) {
    this->pitch_ = nPitch;
    this->yaw_ = nYaw;
}