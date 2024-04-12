#include "GraphicsManager.h"

std::string pModelPaths[] = {
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/dmgHel/DamagedHelmet.gltf",
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Helmet/DamagedHelmet.glb"
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Bistro/bistro.glb"
    //"C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Helmet/FlightHelmet.gltf"
    "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/sponza/Sponza.gltf"
};

std::string skyBoxModelPath_ = "C:/Users/arjoo/OneDrive/Documents/GameProjects/SndBx/SandBox/Cube/cube.gltf";

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
    // CHANGE LAST 2 ARGUMENTS FOR DIFFERENT NUMBER OF MODELS/TEXTURES
    GraphicsManager graphicsManager = GraphicsManager(pModelPaths, skyBoxModelPath_, skyboxTexturePath_, 1, 0);

    graphicsManager.setup();

    // Camera Setup
    graphicsManager.pVkR_->camera_.setVelocity(glm::vec3(0.0f));
    graphicsManager.pVkR_->camera_.setPosition(glm::vec3(1.0f, 0.0f, 0.0f));
    graphicsManager.pVkR_->camera_.setPitchYaw(0.0f, 0.0f);

    bool running = true;
    while (running) {
        // LOGIC LOOP
        // player input -----------
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            graphicsManager.pVkR_->camera_.processSDL(event);
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
        // update camera / player animation -------------
        // 
        // update world objects ------------------
        // 
        // update particles ---------------
        // 
        // update physics -------------------
        // 
        // 
        // includes camera update
        graphicsManager.loopUpdate();
    }

    graphicsManager.shutDown();
    return 0;
}