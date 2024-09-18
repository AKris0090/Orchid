# 🌸ORCHID
![](README_IMAGES/Finals/main.png)  

Orchid is a non-photorealistic forward-rendered game engine made in C++ using Vulkan and PhysX. 

## 🔥Current Feature List:
- [PBR Textures](#PBR-Textures)
- [Image-Based Lighting](#Image-Based-Lighting)
- [Cascaded Shadow Mapping](#Shadow-Mapping--Cascaded-Shadow-Mapping)
- Compute Skinning / Animation
- Frustum Culling
- Physically Based Bloom
- Inverse Hull Outlines
- Nvidia PhysX

## My goals for this project
* Create an engine I can use as a base for a third person rpg game demo
* Have a space I can use to try to implement various graphics programming concepts
* Create a portfolio piece I can use to demonstrate both my game engine and graphics programming knowledge

### PBR Textures
|                                                     Base Color                                                  |                                                     Normal                                                                    |
| :-------------------------------------------------------------------------------------------------------------: | :---------------------------------------------------------------------------------------------------------------------------: |
|                                      ![](README_IMAGES/pbr/basecolor.png)                                       |                                          ![](README_IMAGES/pbr/normal.png)                                                    |
|                                                     Metallic Roughness                                          |                                                     Final Output                                                              |
|                                      ![](README_IMAGES/pbr/metallicroughness.png)                               |                                          ![](README_IMAGES/pbr/combined.png)                                                  |

I implemented the Cook-Torrence BRDF with reference from: https://learnopengl.com/PBR/Theory. When loading a gltf file, my loader implemntation searches for each texture (albedo, normal, metallic/roughness, ao, and emission), and if it cannot find any, a default texture is used. 

### Image-Based Lighting
|                                      Warm Skybox                         |                     Cool Skybox                                          |
| :----------------------------------------------------------------------: | :----------------------------------------------------------------------: |
|                   ![](README_IMAGES/IBL/blaze.png)                       |                    ![](README_IMAGES/IBL/sky.png)                        |

Ambient lighting is entirely controlled by the chosen skybox images. I create 2 cubemaps based on the skybox: an Irradiance cubemap which provides the total diffuse lighting from the skybox, and a Prefiltered Environment cubemap, which filters the skybox based on roughness levels to aid in computation of specular reflections. Each cubemap/image is generated in its own offscreen renderpass.

### Shadow Mapping / Cascaded Shadow Mapping

|                              Shadow Map Slices              |          Final Output                                 |
| :---------------------------------------------------------: | :---------------------------------------------------: |
|                   ![](README_IMAGES/CSM/combined.png)       |           ![](README_IMAGES/CSM/YvUQO8.png)           |

Basic directional light shadowmapping by rendering the scene from the light's perspective. Based on shadow coordinates and the shadow map, if the fragment is visible from the light's perspective, then it is lit accordingly. Cascaded shadow mapping is a technique where you split the camera's view frustrum into smaller frustra, and for each frustra you render a shadow map with its respective center and extents. The subfrustra furthest will have the widest extent (meaning lower level of detail), and the one closer to the camera will have a smaller extent (meaning a higher level of detail). This solves quality issues that come with a single shadow map from a far distance.

### Animations / Compute skinning

https://github.com/AKris0090/Orchid/assets/58652090/a1662d30-31e1-4b9d-9ecd-0a9f098e5afa

https://github.com/AKris0090/Orchid/assets/58652090/cb83ed30-a5de-4a8e-af59-68d82ffc80a4

GLTF files have the ability to store bone data and skinning information, and the gltf loader library I use (tinygltf) has a way to access these. I felt it was beneficial to have the vertex data be of the same format as the other static objects (to reduce overall complicatedness at draw time), so I implemented a compute shader in order to modify the vertex buffer itself, which I then pass through a single rendering pipeline.

### Nvidia PhysX Implementation
I have set up a physics simulation that represents the actors in the scene, and by mapping the transform of the physics objects to the objects in my scene, I can represent physics interactions between them. I have also made use of the built-in character controller class, and plan to implement a third person camera.

## Current Venture
Third person camera using PhysX raycasting
