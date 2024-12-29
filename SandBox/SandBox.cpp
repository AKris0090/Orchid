#include "Scene.h"
#include "Time.h"
#include <chrono>

#define WINDOW_WIDTH 1280.0f
#define WINDOW_HEIGHT 720.0f
#define NEAR_PLANE 0.01f
#define FAR_PLANE 100.0f
#define FOV 75.0f
#define GAMMA
#define EXPOSURE

#define FREE_VAR_1 0.05f;
#define FREE_VAR_2 0.8f
#define BLOOM_RADIUS 0.01f

#define NUM_GAMEOBJECTS 4
#define NUM_ANIMATED_GAMEOBJECTS 1

std::vector<std::string> staticModelPaths = {
    "./dmgHel/DamagedHelmet.gltf",
    "./trainStation/station.gltf",
    "./train/Train-4.glb",
    "./train/leftDoors.glb",
    "./train/rightDoors.glb"
};

std::vector<std::string> animatedModelPaths = {
   "./goro/goroWalk2.glb",
};

std::vector<std::string> skyboxTexturePaths = {
    "./Cube/blaze2/nx.png",
    "./Cube/blaze2/px.png",
    "./Cube/blaze2/py.png",
    "./Cube/blaze2/ny.png",
    "./Cube/blaze2/pz.png",
    "./Cube/blaze2/nz.png"
};

std::string skyboxModelPath = "./Cube/cube.glb";

using namespace std::literals;

void Scene::setupScene() {
    GameObject* helmet = new GameObject();
    helmet->isDynamic = true;
    helmet->addRenderTarget(&graphicsManager.vkR_.staticRenderTargets[0]);
    helmet->renderTargetTransforms[0]->position = glm::vec4(2.25f, 40.0f, 0.0f, 0.0f);
    physicsManager.addCubeToGameObject(helmet, physx::PxVec3(helmet->renderTargetTransforms[0]->position.x, helmet->renderTargetTransforms[0]->position.y, helmet->renderTargetTransforms[0]->position.z), 0.85f);
    this->graphicsManager.staticGameObjects.push_back(helmet);

    std::cout << "generated helmet physics" << std::endl;

    GameObject* station = new GameObject();
    station->addRenderTarget(&graphicsManager.vkR_.staticRenderTargets[1]);
    station->renderTargetTransforms[0]->scale = glm::vec3(0.01f);
    physicsManager.addShapeToGameObject(station, physx::PxVec3(0, 0, 0), graphicsManager.vkR_.vertices_, graphicsManager.vkR_.indices_, glm::vec3(1.0f));
    this->graphicsManager.staticGameObjects.push_back(station);

    std::cout << "generated station physics" << std::endl;

    TrainObject* rightTrain = new TrainObject();
    rightTrain->isDynamic = true;
    rightTrain->addRenderTarget(&graphicsManager.vkR_.staticRenderTargets[2]);
    rightTrain->addRenderTarget(&graphicsManager.vkR_.staticRenderTargets[3]);
    rightTrain->addRenderTarget(&graphicsManager.vkR_.staticRenderTargets[4]);
    Transform rightTrainStartTransform;
    rightTrainStartTransform.position = glm::vec3(-50.0f, 0.0f, 0.0f);
    rightTrain->setup(rightTrainStartTransform, glm::vec3(0.0f, 0.0f, 0.0f), 10000, 5000, 1500, 10000, 1);
    rightTrain->updatePosition();
    this->graphicsManager.staticGameObjects.push_back(rightTrain);

    TrainObject* leftTrain = new TrainObject();
    leftTrain->isDynamic = true;
    leftTrain->addRenderTarget(&graphicsManager.vkR_.staticRenderTargets[2]);
    leftTrain->addRenderTarget(&graphicsManager.vkR_.staticRenderTargets[3]);
    leftTrain->addRenderTarget(&graphicsManager.vkR_.staticRenderTargets[4]);
    Transform leftTrainStartTransform;
    leftTrainStartTransform.position = glm::vec3(50.0f, 0.0f, 6.2f);
    leftTrainStartTransform.rotation = glm::vec3(0.0f, PI, 0.0f);
    leftTrain->setup(leftTrainStartTransform, glm::vec3(5.5f, 0.0f, 6.2f), 10000, 5000, 1500, 10000, -1);
    leftTrain->updatePosition();
    this->graphicsManager.staticGameObjects.push_back(leftTrain);

    PlayerObject* player = new PlayerObject();
    player->isDynamic = true;
    player->addRenderTarget(&graphicsManager.vkR_.animatedRenderTargets[0]);
    player->setup(physicsManager.pMaterial, physicsManager.pScene, &graphicsManager.vkR_.camera_);
    player->walkAnim = player->renderTargets[0]->baseAnim;
    player->transform.rotation = glm::vec3(PI / 2.0f, 0.0f, 0.0f);
    player->transform.scale = glm::vec3(0.00615f, 0.00615f, 0.00615f);
    player->characterController->setFootPosition(physx::PxExtendedVec3(0.0, 0.0, 0.0));
    player->currentState = PLAYERSTATE::IDLE;
    player->runAnim.loadAnimation(std::string("./goro/goroRun2.glb"), player->renderTargets[0]->pParentNodes);
    player->idleAnim.loadAnimation(std::string("./goro/goroIdle.glb"), player->renderTargets[0]->pParentNodes);
    player->renderTargets[0]->src = new std::vector<AnimatedGLTFObj::secondaryTransform>(player->walkAnim.numChannels);
    player->renderTargets[0]->dst = new std::vector<AnimatedGLTFObj::secondaryTransform>(player->walkAnim.numChannels);
    player->activeAnimation = &(player->idleAnim);
    player->previousAnimation = &(player->idleAnim);
    player->smoothDuration = 150ms;
    player->smoothAmount = FLT_MAX;
    this->graphicsManager.animatedGameObjects.push_back(player);
    this->player = player;

    graphicsManager.vkR_.capHeight = player->cap_height;

    graphicsManager.vkR_.updateBindMatrices();

    graphicsManager.vkR_.addToDrawCalls({0, 1, 2, 3, 4, 2, 3, 4}, {0});

    graphicsManager.vkR_.createDrawCallBuffer();

    graphicsManager.vkR_.createModelMatrixBuffer();

    graphicsManager.vkR_.createComputeCullResources(graphicsManager.vkR_.numFramesInFlight);

    graphicsManager.vkR_.vertices_.clear();
    graphicsManager.vkR_.vertices_.shrink_to_fit();
    graphicsManager.vkR_.indices_.clear();
    graphicsManager.vkR_.indices_.shrink_to_fit();
}

int main(int argc, char* argv[]) {
    std::cout << std::filesystem::current_path() << std::endl;

    Time::setInitialTime();

    Scene mainScene;
    mainScene.graphicsManager = GraphicsManager(WINDOW_WIDTH, WINDOW_HEIGHT);
    DirectionalLight light = DirectionalLight(glm::vec3(20.0f, 40.0f, 8.0f));
    mainScene.graphicsManager.vkR_.pDirectionalLight_ = &light;
    mainScene.graphicsManager.vkR_.depthBias_ = 0.05f;
    mainScene.graphicsManager.vkR_.camera_ = FPSCam(NEAR_PLANE, FAR_PLANE, WINDOW_WIDTH, WINDOW_HEIGHT, FOV);
    mainScene.graphicsManager.vkR_.maxReflectionLOD_ = 7.0f;
    mainScene.graphicsManager.vkR_.gamma_ = 1.5f;
    mainScene.graphicsManager.vkR_.exposure_ = 12.5f;
    mainScene.graphicsManager.vkR_.specularCont = 0.05f;
    mainScene.graphicsManager.vkR_.nDotVSpec = 0.8f;
    mainScene.graphicsManager.vkR_.bloomRadius = 0.01f;
    mainScene.graphicsManager.setup(staticModelPaths, animatedModelPaths, skyboxModelPath, skyboxTexturePaths);

    mainScene.physicsManager = PhysicsManager();
    mainScene.physicsManager.setup();

    mainScene.setupScene();

    bool mousemode_ = true;
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {

            // player input -----------------
            if (mousemode_ == true) {
                mainScene.graphicsManager.vkR_.camera_.processSDL(event);
            }
            Input::handleSDLInput(event);
            ImGui_ImplSDL2_ProcessEvent(&event);


            switch (event.type) {
            case SDL_QUIT:
                running = false;
                break;
            case SDL_WINDOWEVENT_RESIZED:
                mainScene.graphicsManager.vkR_.frBuffResized_ = true;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    mousemode_ = !mousemode_;
                    if (mousemode_ == false) {
                        SDL_SetRelativeMouseMode(SDL_FALSE);
                    }
                    else {
                        SDL_SetRelativeMouseMode(SDL_TRUE);
                    }
                }
                else if (event.key.keysym.sym == SDLK_TAB) {
                    mainScene.graphicsManager.vkR_.camera_.isAttatched = !mainScene.graphicsManager.vkR_.camera_.isAttatched;
                }
                break;
            default:
                break;
            }
        }

        // Time/frame update ---------
        Time::updateTime();

        // position/animation update -----------
        mainScene.loopUpdate();

        // update camera ------------
        if (mainScene.graphicsManager.vkR_.camera_.isAttatched) {
            mainScene.graphicsManager.vkR_.camera_.physicsUpdate(mainScene.player->transform, mainScene.physicsManager.pScene, mainScene.player->characterController, mainScene.player->cap_height);
        }
        else {
            mainScene.graphicsManager.vkR_.camera_.update();
        }

        // update physics -------------------
        mainScene.physicsManager.loopUpdate(mainScene.graphicsManager.staticGameObjects, mainScene.graphicsManager.animatedGameObjects, Time::getDeltaTime());
        
        // update graphics -------------------
        mainScene.graphicsManager.loopUpdate();
    }

    return 0;
}