#include "GraphicsManager.h"
#include "PhysicsManager.h"
#include "Time.h"
#include <chrono>

#define WINDOW_WIDTH 1280.0f
#define WINDOW_HEIGHT 720.0f

std::vector<std::string> staticModelPaths = {
    "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/dmgHel/DamagedHelmet.gltf",
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Helmet/DamagedHelmet.glb"
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Bistro/bistro.glb"
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Helmet/FlightHelmet.gltf"
    
    "C:/Users/arjoo/Downloads/abandoned_underground_train_station.glb",
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/axis/Answer Arena.glb"
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/sponza/Sponza.gltf",
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Bistro/terrain_gridlines.glb"
};

std::vector<std::string> animatedModelPaths = {
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/emily/Emily_Walk.glb",
   //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/wolf_animated/Wolf-2.glb"
   //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/goro/goro.glb"
   "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/goro/goroWalk2.glb"
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/dmgHel/DamagedHelmet.gltf",
};

std::vector<std::string> skyboxTexturePaths = {
    "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/skymap/right.jpg",
    "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/skymap/left.jpg",
    "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/skymap/top.jpg",
    "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/skymap/bottom.jpg",
    "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/skymap/front.jpg",
    "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/skymap/back.jpg"

    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/blaze/negx.bmp",
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/blaze/posx.bmp",
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/blaze/posy.bmp",
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/blaze/negy.bmp",
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/blaze/posz.bmp",
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/blaze/negz.bmp"

    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/cubemap/posx.jpg",
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/cubemap/negx.jpg",
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/cubemap/posy.jpg",
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/cubemap/negy.jpg",
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/cubemap/posz.jpg",
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/cubemap/negz.jpg"

    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/blaze2/nx.png",
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/blaze2/px.png",
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/blaze2/py.png",
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/blaze2/ny.png",
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/blaze2/pz.png",
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/blaze2/nz.png"

    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/pD/DiffuseTexture4.png",
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/pD/DiffuseTexture4.png",
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/pD/DiffuseTexture4.png",
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/pD/DiffuseTexture4.png",
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/pD/DiffuseTexture4.png",
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/pD/DiffuseTexture4.png"
};

std::string skyboxModelPath = "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/cube.glb";

int main(int argc, char* argv[]) {
    GraphicsManager graphicsManager = GraphicsManager(staticModelPaths, animatedModelPaths, skyboxModelPath, skyboxTexturePaths, WINDOW_WIDTH, WINDOW_HEIGHT);
    graphicsManager.pVkR_ = new VulkanRenderer();

    //graphicsManager.pVkR_->pDirectionalLight_ = new DirectionalLight(glm::vec3(50.0f, 40.0f, 2.0f));
    graphicsManager.pVkR_->pDirectionalLight_ = new DirectionalLight(glm::vec3(20.0f, 40.0f, 8.0f));
    graphicsManager.pVkR_->depthBias_ = 0.003f;
    graphicsManager.pVkR_->camera_.setNearPlane(0.01f);
    graphicsManager.pVkR_->camera_.setFarPlane(400.0f);
    graphicsManager.pVkR_->camera_.setFOV(glm::radians(75.0f));
    graphicsManager.pVkR_->camera_.setAspectRatio(WINDOW_WIDTH / WINDOW_HEIGHT);
    graphicsManager.pVkR_->maxReflectionLOD_ = 7.0f;
    graphicsManager.pVkR_->gamma_ = 1.0f;
    graphicsManager.pVkR_->exposure_ = 0.5f;
    graphicsManager.pVkR_->applyTonemap = true;
    graphicsManager.pVkR_->specularCont = 4.0f;
    graphicsManager.pVkR_->nDotVSpec = 1.0f;
    graphicsManager.pVkR_->bloomRadius = 0.00001f;

    PhysicsManager physicsManager = PhysicsManager();

    // Camera Setup
    graphicsManager.pVkR_->camera_.setVelocity(glm::vec3(0.0f));
    graphicsManager.pVkR_->camera_.setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    graphicsManager.pVkR_->camera_.setPitchYaw(0.0f, 0.0f);

    graphicsManager.setup();
    physicsManager.setup();

    graphicsManager.pVkR_->updateBindMatrices();

    graphicsManager.gameObjects[0]->isDynamic = true; // helmet

    glm::vec3 scale = glm::vec3(0.01f);
    //glm::vec3 scale = glm::vec3(0.75f);
    graphicsManager.gameObjects[1]->transform.scale = scale;

    for (GameObject* g : graphicsManager.gameObjects) {
        g->renderTarget->localModelTransform = g->transform.to_matrix();
    }

    // second helmet
    //------graphicsManager.animatedObjects[1]->transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
    //------graphicsManager.animatedObjects[1]->transform.rotation = glm::vec3(PI / 2.0f, 0.0f, 0.0f);

    graphicsManager.animatedObjects[0]->isPlayerObj = true;

    // WOLF
     //graphicsManager.animatedObjects[0]->transform.rotation = glm::vec3(0.0f, PI / 2.0f, 0.0f);
     //graphicsManager.animatedObjects[0]->transform.scale = glm::vec3(2.0f, 2.0f, 2.0f);
     
    graphicsManager.animatedObjects[0]->transform.rotation = glm::vec3(PI / 2.0f, 0.0f, 0.0f);
    graphicsManager.animatedObjects[0]->transform.scale = glm::vec3(0.0075f, 0.0075f, 0.0075f);
    graphicsManager.animatedObjects[0]->renderTarget->runAnim.loadAnimation(std::string("C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/goro/goroRun2.glb"), graphicsManager.animatedObjects[0]->renderTarget->pParentNodes);
    graphicsManager.animatedObjects[0]->renderTarget->idleAnim.loadAnimation(std::string("C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/goro/goroIdle.glb"), graphicsManager.animatedObjects[0]->renderTarget->pParentNodes);
    graphicsManager.animatedObjects[0]->activeAnimation = &(graphicsManager.animatedObjects[0]->renderTarget->idleAnim);
    graphicsManager.animatedObjects[0]->previousAnimation = &(graphicsManager.animatedObjects[0]->renderTarget->idleAnim);
    //graphicsManager.animatedObjects[0]->transform.rotation = glm::vec3(PI / 2, 0.0f, 0.0f);
    //graphicsManager.animatedObjects[0]->transform.scale = glm::vec3(0.01f, 0.01f, 0.01f);
    //graphicsManager.animatedObjects[0]->transform.position = glm::vec3(1.0f, 0.0f, 0.0f);

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
    //physicsManager.addPlane();
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

        // update camera ------------
        if (graphicsManager.pVkR_->camera_.isAttatched) {
            graphicsManager.pVkR_->camera_.physicsUpdate(player->transform, physicsManager.pScene, player->characterController, player->cap_height);
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