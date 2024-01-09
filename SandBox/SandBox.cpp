#include "GraphicsManager.h"

std::string model_paths[] = {
    /*"VikingRoom/OBJ.obj",
    "GSX/Srad 750.obj"*/
    //"Helmet/DamagedHelmet.glb"
    //"Helmet/FlightHelmet.gltf"
    //"sp/NewSponza_Main_glTF_002.gltf",
    "sponza/Sponza.gltf"
    //"curt/NewSponza_Curtains_glTF.gltf"
};

std::string texture_paths[] = {
    "GSX/GSX.png",
    "VikingRoom/Material.png"
};

//#define NUM_MODELS 2;
//#define NUM_TEXTURES 0;

int main(int argc, char* argv[]) {

    // CHANGE LAST 2 ARGUMENTS FOR DIFFERENT NUMBER OF MODELS/TEXTURES
    GraphicsManager graphicsManager = GraphicsManager(model_paths, texture_paths, 1, 0);

    graphicsManager.setup();

    bool running = true;
    while (running) {
        // LOGIC LOOP
        // player input -----------
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            graphicsManager.vkR->camera.processSDL(event);
            ImGui_ImplSDL2_ProcessEvent(&event);
            switch (event.type) {
            case SDL_QUIT:
                running = false;
                break;
            case SDL_WINDOWEVENT_RESIZED:
                graphicsManager.vkR->frBuffResized = true;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_TAB) {
                    graphicsManager.mousemode = !graphicsManager.mousemode;
                    if (graphicsManager.mousemode == false) {
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