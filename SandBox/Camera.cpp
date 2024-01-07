#include "Camera.h"

void FPSCam::update() {
	glm::mat4 camrotation = getRotationMatrix();
	position += glm::vec3(camrotation * glm::vec4(velocity * 0.5f, 0.0f));
}

void FPSCam::processSDL(SDL_Event& event) {
    if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_w) { this->velocity.z = -0.0015f; }
        if (event.key.keysym.sym == SDLK_s) { this->velocity.z = 0.0015f; }
        if (event.key.keysym.sym == SDLK_a) { this->velocity.x = -0.0015f; }
        if (event.key.keysym.sym == SDLK_d) { this->velocity.x = 0.0015f; }
        if (event.key.keysym.sym == SDLK_e) { this->velocity.y = 0.0015f; }
        if (event.key.keysym.sym == SDLK_q) { this->velocity.y = -0.0015f; }
    }

    if (event.type == SDL_KEYUP) {
        if (event.key.keysym.sym == SDLK_w) { this->velocity.z = 0.0f; }
        if (event.key.keysym.sym == SDLK_s) { this->velocity.z = 0.0f; }
        if (event.key.keysym.sym == SDLK_a) { this->velocity.x = 0.0f; }
        if (event.key.keysym.sym == SDLK_d) { this->velocity.x = 0.0f; }
        if (event.key.keysym.sym == SDLK_e) { this->velocity.y = 0.0f; }
        if (event.key.keysym.sym == SDLK_q) { this->velocity.y = 0.0f; }
    }

    if (event.type == SDL_MOUSEMOTION) {
        this->yaw += (float)event.motion.xrel / 300.f;
        this->pitch -= (float)event.motion.yrel / 300.f;
    }
}

glm::mat4 FPSCam::getViewMatrix() {
    // to have a real camera matrix, you dont rotate/move the camera, but you move/rotate the world in the opposite direction of camera motion. Matrices are accumulated by shaders (VKGuide)
    glm::mat4 camTranslation = glm::translate(glm::mat4(1.0f), this->position);
    glm::mat4 camRotation = this->getRotationMatrix();

    this->viewPos = glm::vec4(this->position, 0.0f) * glm::vec4(-1.0f, 1.0f, -1.0f, 1.0f);

    return glm::inverse(camTranslation * camRotation);
}

glm::mat4 FPSCam::getRotationMatrix() {
    glm::quat pitchRotation = glm::angleAxis(this->pitch, glm::vec3{ 1.0f, 0.0f, 0.0f });
    // inverse left-right, since left is negative x
    glm::quat yawRotation = glm::angleAxis(this->yaw, glm::vec3{ 0.0f, -1.0f, 0.0f });

    return (glm::toMat4(yawRotation) * glm::toMat4(pitchRotation));
}

void FPSCam::setPosition(glm::vec3 pos) {
    this->position = pos;
}

void FPSCam::setVelocity(glm::vec3 vel) {
    this->velocity = vel;
}

void FPSCam::setPitchYaw(float nPitch, float nYaw) {
    this->pitch = nPitch;
    this->yaw = nYaw;
}