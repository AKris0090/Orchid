#include "GraphicsManager.h"
#include "PhysicsManager.h"
#include <chrono>

std::string pModelPaths[] = {
    "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/dmgHel/DamagedHelmet.gltf",
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Helmet/DamagedHelmet.glb"
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Bistro/bistro.glb"
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Helmet/FlightHelmet.gltf"
    "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/sponza/Sponza.gltf"
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Bistro/terrain_gridlines.glb"
};

std::vector<std::string> skyboxTexturePath_ = {
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/skymap/right.jpg",
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/skymap/left.jpg",
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/skymap/top.jpg",
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/skymap/bottom.jpg",
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/skymap/front.jpg",
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/skymap/back.jpg"

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

    "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/blaze2/nx.png",
    "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/blaze2/px.png",
    "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/blaze2/py.png",
    "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/blaze2/ny.png",
    "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/blaze2/pz.png",
    "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/blaze2/nz.png"
};

int main(int argc, char* argv[]) {
    float deltaTime = 1.0f / 144.0f;
    // CHANGE LAST 2 ARGUMENTS FOR DIFFERENT NUMBER OF MODELS/TEXTURES
    GraphicsManager graphicsManager = GraphicsManager(pModelPaths,
                                                      "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/cube.gltf",
                                                      skyboxTexturePath_,
                                                      2, // num models IMPORTANT
                                                      0); // num textures

    PhysicsManager physicsManager = PhysicsManager();

    graphicsManager.setup();
    graphicsManager.pVkR_->gameObjects = graphicsManager.gameObjects;
    physicsManager.setup();

    // Camera Setup
    graphicsManager.pVkR_->camera_.setVelocity(glm::vec3(0.0f));
    graphicsManager.pVkR_->camera_.setPosition(glm::vec3(1.0f, 0.0f, 0.0f));
    graphicsManager.pVkR_->camera_.setPitchYaw(0.0f, 0.0f);

    // Scene objects
    graphicsManager.gameObjects[1]->transform.scale = glm::vec3(0.008f);

    graphicsManager.gameObjects[0]->transform.rotation = glm::vec3(PI / 2, 0.0f, 0.0f);

    physicsManager.addCubeToGameObject(graphicsManager.gameObjects[0], physx::PxVec3(0, 40, 0), 0.85f);
    physicsManager.addShapeToGameObject(graphicsManager.gameObjects[1], physx::PxVec3(0, 0, 0), graphicsManager.gameObjects[1]->transform.scale);

    for (GameObject* g : graphicsManager.gameObjects) {
        g->renderTarget->modelTransform = g->transform.to_matrix();
    }

    //graphicsManager.animatedObjects[0]->transform.rotation = glm::vec3(0.0f, PI / 2.0f, 0.0f);
    graphicsManager.animatedObjects[0]->transform.rotation = glm::vec3(PI / 2, 0.0f, 0.0f);
    //graphicsManager.animatedObjects[0]->transform.scale = glm::vec3(2.0f, 2.0f, 2.0f);
    graphicsManager.animatedObjects[0]->transform.scale = glm::vec3(0.01f, 0.01f, 0.01f);
    graphicsManager.animatedObjects[0]->transform.position = glm::vec3(1.0f, 0.0f, 0.0f);

    for (AnimatedGameObject* g : graphicsManager.animatedObjects) {
        g->renderTarget->modelTransform = g->transform.to_matrix();
    }

    // Player setup
    PlayerObject* player = new PlayerObject(physicsManager.pMaterial, physicsManager.pScene);
    player->characterController->setFootPosition(physx::PxExtendedVec3(0.0, 0.5, 0.0));

    bool running = true;
    while (running) {
        // Core SDL Loop
        SDL_Event event;
        while (SDL_PollEvent(&event)) {

            // player input -----------------
            graphicsManager.pVkR_->camera_.processSDL(event);
            Input::handleSDLInput(event);
            ImGui_ImplSDL2_ProcessEvent(&event);


            switch (event.type) {
            case SDL_QUIT:
                running = false;
                break;
            case SDL_WINDOWEVENT_RESIZED:
                graphicsManager.pVkR_->frBuffResized_ = true;
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
            default:
                break;
            }
        }

        // player animation -------------
        if (Input::leftMouseDown()) {
            graphicsManager.pVkR_->animatedObjects[0]->renderTarget->updateAnimation(deltaTime);
        }
        // update particles ---------------
        // 
        // update physics -------------------
        physicsManager.loopUpdate(graphicsManager.gameObjects, player, &(graphicsManager.pVkR_->camera_), deltaTime);
        // 
        // update graphics -------------------
        graphicsManager.loopUpdate();
    }

    graphicsManager.shutDown();
    return 0;
}