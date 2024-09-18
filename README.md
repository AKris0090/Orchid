# 🌸 ORCHID
![](README_IMAGES/Finals/main.png)  

Orchid is a non-photorealistic forward-rendered game engine made in C++ using Vulkan and PhysX. 

## 🔥 Feature Highlights:
- [PBR Textures](#PBR-Textures)
- [Image-Based Lighting](#Image-Based-Lighting)
- [Cascaded Shadow Mapping](#Shadow-Mapping--Cascaded-Shadow-Mapping)
- Compute Skinning / Animation
- Frustum Culling
- Physically Based Bloom
- Inverse Hull Outlines
- Nvidia PhysX

## 🎯 My goals for this project
* Create an engine I can use as a base for a third person rpg game demo
* Have a space I can use to try to implement various graphics programming concepts
* Create a portfolio piece I can use to demonstrate both my game engine and graphics programming knowledge

### PBR Textures
|                                                     Base Color                                                  |                                                     Normal                                                                    |
| :-------------------------------------------------------------------------------------------------------------: | :---------------------------------------------------------------------------------------------------------------------------: |
|                                      ![](README_IMAGES/pbr/basecolor.png)                                       |                                          ![](README_IMAGES/pbr/normal.png)                                                    |
|                                                     Metallic Roughness                                          |                                                     Final Output                                                              |
|                                      ![](README_IMAGES/pbr/metallicroughness.png)                               |                                          ![](README_IMAGES/pbr/combined.png)                                                  |

I implemented the Cook-Torrence BRDF with reference from: https://learnopengl.com/PBR/Theory. My loader implemntation searches for essential textures: albedo, normal, metallic/roughness, ambient occlusion, and emission. If one is not found, a default texture is used. 

### Image-Based Lighting
|                                      Warm Skybox                         |                     Cool Skybox                                          |
| :----------------------------------------------------------------------: | :----------------------------------------------------------------------: |
|                   ![](README_IMAGES/IBL/blaze.png)                       |                    ![](README_IMAGES/IBL/sky.png)                        |

The ambient lighting is driven by the chosen skybox images. Two cubemaps are generated:

1. Irradiance Cubemap for diffuse lighting.
2. Prefiltered Environment Cubemap for specular reflections, filtered by roughness.

Each cubemap is generated with offscreen render passes.

### Shadow Mapping / Cascaded Shadow Mapping

|                              Shadow Map Slices              |          Final Output                                 |
| :---------------------------------------------------------: | :---------------------------------------------------: |
|                   ![](README_IMAGES/CSM/combined.png)       |           ![](README_IMAGES/CSM/YvUQO8.png)           |

Cascaded Shadow Mapping (CSM) splits the camera’s view frustum into smaller sections, allowing detailed shadows close to the camera while reducing detail farther away. This technique solves shadow quality issues in scenes with distant light sources.

### Animations / Compute skinning

https://github.com/AKris0090/Orchid/assets/58652090/a1662d30-31e1-4b9d-9ecd-0a9f098e5afa

https://github.com/AKris0090/Orchid/assets/58652090/cb83ed30-a5de-4a8e-af59-68d82ffc80a4

GLTF files have the ability to store bone data and skinning information, and the gltf loader library I use (tinygltf) has a way to access these. I felt it was beneficial to have the vertex data be of the same format as the other static objects (to reduce overall complicatedness at draw time), so I implemented a compute shader in order to modify the vertex buffer itself, which I then pass through a single rendering pipeline.

### Nvidia PhysX Implementation
I have set up a physics simulation that represents the actors in the scene, and by mapping the transform of the physics objects to the objects in my scene, I can represent physics interactions between them. I have also made use of the built-in character controller class, and plan to implement a third person camera.

## Current Venture
Third person camera using PhysX raycasting
