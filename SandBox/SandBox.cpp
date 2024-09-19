#include "GraphicsManager.h"
#include "PhysicsManager.h"
#include "Time.h"
#include <chrono>

#define WINDOW_WIDTH 1280.0f
#define WINDOW_HEIGHT 720.0f

std::vector<std::string> staticModelPaths = {
    "./dmgHel/DamagedHelmet.gltf",
    "./trainStation/station.gltf",
    "./train/Train-4.glb",
    "./train/leftDoors.glb",
    "./train/rightDoors.glb",
    "./train/turnedTrain.glb",
    "./train/turnedRightDoors.glb",
    "./train/turnedLeftDoors.glb"
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

int main(int argc, char* argv[]) {
    std::cout << std::filesystem::current_path() << std::endl;

    GraphicsManager graphicsManager = GraphicsManager(staticModelPaths, animatedModelPaths, skyboxModelPath, skyboxTexturePaths, WINDOW_WIDTH, WINDOW_HEIGHT);
    graphicsManager.pVkR_ = new VulkanRenderer();

    graphicsManager.pVkR_->pDirectionalLight_ = new DirectionalLight(glm::vec3(20.0f, 40.0f, 8.0f));
    graphicsManager.pVkR_->depthBias_ = 0.05f;
    graphicsManager.pVkR_->camera_.setNearPlane(0.01f);
    graphicsManager.pVkR_->camera_.setFarPlane(100.0f);
    graphicsManager.pVkR_->camera_.setFOV(glm::radians(75.0f));
    graphicsManager.pVkR_->camera_.setAspectRatio(WINDOW_WIDTH / WINDOW_HEIGHT);
    graphicsManager.pVkR_->maxReflectionLOD_ = 7.0f;
    graphicsManager.pVkR_->gamma_ = 1.5f;
    graphicsManager.pVkR_->exposure_ = 12.5f;
    graphicsManager.pVkR_->specularCont = 0.05f;
    graphicsManager.pVkR_->nDotVSpec = 0.8f;
    graphicsManager.pVkR_->bloomRadius = 0.00001f;
    graphicsManager.pVkR_->camera_.setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    graphicsManager.pVkR_->camera_.setPitchYaw(0.0f, 0.0f);

    PhysicsManager physicsManager = PhysicsManager();

    graphicsManager.setup();
    physicsManager.setup();

    graphicsManager.pVkR_->updateBindMatrices();

    graphicsManager.gameObjects[0]->isDynamic = true; // helmet

    glm::vec3 scale = glm::vec3(0.01f);
    //glm::vec3 scale = glm::vec3(1.0f);
    graphicsManager.gameObjects[1]->transform.scale = scale;

    for (GameObject* g : graphicsManager.gameObjects) {
        g->renderTarget->localModelTransform = g->transform.to_matrix();
    }

    graphicsManager.animatedObjects[0]->isPlayerObj = true;
     
    graphicsManager.animatedObjects[0]->transform.rotation = glm::vec3(PI / 2.0f, 0.0f, 0.0f);
    graphicsManager.animatedObjects[0]->transform.scale = glm::vec3(0.00615f, 0.00615f, 0.00615f);
    graphicsManager.animatedObjects[0]->renderTarget->runAnim.loadAnimation(std::string("./goro/goroRun2.glb"), graphicsManager.animatedObjects[0]->renderTarget->pParentNodes);
    graphicsManager.animatedObjects[0]->renderTarget->idleAnim.loadAnimation(std::string("./goro/goroIdle.glb"), graphicsManager.animatedObjects[0]->renderTarget->pParentNodes);
    graphicsManager.animatedObjects[0]->activeAnimation = &(graphicsManager.animatedObjects[0]->renderTarget->idleAnim);
    graphicsManager.animatedObjects[0]->previousAnimation = &(graphicsManager.animatedObjects[0]->renderTarget->idleAnim);

    for (AnimatedGameObject* g : graphicsManager.animatedObjects) {
        g->renderTarget->localModelTransform = g->transform.to_matrix();
    }

    // Model Matrices TODO: MOVE THIS SOMEWHERE ELSE LOL
    graphicsManager.pVkR_->addToDrawCalls();
    graphicsManager.pVkR_->createDrawCallBuffer();
    graphicsManager.pVkR_->createModelMatrixBuffer(MAX_FRAMES_IN_FLIGHT);
    graphicsManager.pVkR_->createComputeCullResources(MAX_FRAMES_IN_FLIGHT);
   
    physicsManager.addCubeToGameObject(graphicsManager.gameObjects[0], physx::PxVec3(2.25, 40, 0), 0.85f);
    scale = glm::vec3(1.0f);
    physicsManager.addShapeToGameObject(graphicsManager.gameObjects[1], physx::PxVec3(0, 0, 0), graphicsManager.pVkR_->vertices_, graphicsManager.pVkR_->indices_, scale);

    graphicsManager.pVkR_->vertices_.clear();
    graphicsManager.pVkR_->vertices_.shrink_to_fit();

    // Player setup
    PlayerObject* player = new PlayerObject(physicsManager.pMaterial, physicsManager.pScene);
    player->playerGameObject = graphicsManager.animatedObjects[0];
    player->characterController->setFootPosition(physx::PxExtendedVec3(0.0, 0.0, 0.0));
    player->transform.scale = graphicsManager.animatedObjects[0]->transform.scale;
    player->playerGameObject = graphicsManager.animatedObjects[0];
    player->currentState = PLAYERSTATE::IDLE;

    // Right Train setup
    TrainObject* rightTrain = new TrainObject(glm::vec3(-50.0f, 0.0f, 0.0f), 10000, 5000, 1500, 10000);
    rightTrain->trainBodyObject = graphicsManager.gameObjects[2];
    rightTrain->trainLeftDoorObject = graphicsManager.gameObjects[3];
    rightTrain->trainRightDoorObject = graphicsManager.gameObjects[4];
    rightTrain->updatePosition();

    TrainObject* leftTrain = new TrainObject(glm::vec3(50.0f, 0.0f, 0.0f), 10000, 5000, 1500, 10000);
    leftTrain->transform.rotation = glm::vec3(0.0f, PI / 2.0f, 0.0f);
    leftTrain->trainBodyObject = graphicsManager.gameObjects[5];
    leftTrain->trainLeftDoorObject = graphicsManager.gameObjects[6];
    leftTrain->trainRightDoorObject = graphicsManager.gameObjects[7];
    leftTrain->updatePosition();

    graphicsManager.pVkR_->capHeight = player->cap_height;

    graphicsManager.player = player;

    bool running = true;
    while (running) {
        // Core SDL Loop
        SDL_Event event;
        while (SDL_PollEvent(&event)) {

            // player input -----------------
            if (graphicsManager.mousemode_ == true) {
                graphicsManager.pVkR_->camera_.processSDL(event);
            }
            Input::handleSDLInput(event);
            ImGui_ImplSDL2_ProcessEvent(&event);


            switch (event.type) {
            case SDL_QUIT:
                running = false;
                break;
            case SDL_WINDOWEVENT_RESIZED:
                graphicsManager.pVkR_->frBuffResized_ = true;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    graphicsManager.mousemode_ = !graphicsManager.mousemode_;
                    if (graphicsManager.mousemode_ == false) {
                        SDL_SetRelativeMouseMode(SDL_FALSE);
                    }
                    else {
                        SDL_SetRelativeMouseMode(SDL_TRUE);
                    }
                }
                else if (event.key.keysym.sym == SDLK_TAB) {
                    graphicsManager.pVkR_->camera_.isAttatched = !graphicsManager.pVkR_->camera_.isAttatched;
                }
                break;
            default:
                break;
            }
        }

        // Time/frame update ---------
        Time::updateTime();

        // player update -----------
        player->loopUpdate(&(graphicsManager.pVkR_->camera_));
        rightTrain->loopUpdate();
        leftTrain->loopUpdate();

        // update camera ------------
        if (graphicsManager.pVkR_->camera_.isAttatched) {
            graphicsManager.pVkR_->camera_.physicsUpdate(player->transform, physicsManager.pScene, player->characterController, player->cap_height);
            graphicsManager.pVkR_->playerPosition = player->transform.position;
        }
        else {
            graphicsManager.pVkR_->camera_.update();
        }

        // player animation -------------

        graphicsManager.animatedObjects[0]->updateAnimation(graphicsManager.pVkR_->inverseBindMatrices, Time::getDeltaTime());
        graphicsManager.pVkR_->updateBindMatrices();

        // update physics -------------------
        // includes game object position updates TODO: REMOVE FROM HERE
        physicsManager.loopUpdate(graphicsManager.animatedObjects[0], graphicsManager.gameObjects, graphicsManager.animatedObjects, player, &(graphicsManager.pVkR_->camera_), Time::getDeltaTime());
        graphicsManager.pVkR_->updateModelMatrices();
        
        // update graphics -------------------
        graphicsManager.loopUpdate();
    }

    graphicsManager.shutDown();
    return 0;
}