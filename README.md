# ORCHID

## My goals for this project
* Create an engine I can use as a base for a third person shooter game demo
* Have a space I can use to try to implement various graphics programming concepts
* Create a portfolio piece I can use to both demonstrate my game engine and graphics programming knowledge

## Current Features
### PBR Textures
|                                                     Base Color                                                  |                                                     Normal                                                                    |
| :-------------------------------------------------------------------------------------------------------------: | :---------------------------------------------------------------------------------------------------------------------------: |
|                                      ![](README_IMAGES/pbr/basecolor.png)                                       |                                          ![](README_IMAGES/pbr/normal.png)                                                    |
|                                                     Metallic Roughness                                          |                                                     Final Output                                                              |
|                                      ![](README_IMAGES/pbr/metallicroughness.png)                               |                                          ![](README_IMAGES/pbr/combined.png)                                                  |

I implemented the Cook-Torrence BRDF with reference from: https://learnopengl.com/PBR/Theory. When loading a gltf file, my loader implemntation searches for each texture (albedo, normal, metallic/roughness, ao, and emission), and if it cannot find any, a default texture is used. 

### Shadow Mapping
|                                      Shadow Map                                                                 |                                          Fragment Shader Output                                                               |
| :-------------------------------------------------------------------------------------------------------------: | :---------------------------------------------------------------------------------------------------------------------------: |
|                                      ![](README_IMAGES/shadow/shadowmap-2.png)                                  |                                          ![](README_IMAGES/shadow/shadowmap-1.png)                                            |

Basic area light shadowmapping by rendering the scene from the light's perspective. This texture is then passed into the fragment shader where a shadow coordinate is calculated by multiplying the world coordinates by the light's View * Projection matrix. Based on these coordinates and the shadow map, it is determined whether or not the fragment is visible or not from the light's point of view, and that decides if the fragment is lit or shaded. Currently working on cascaded shadow mapping, where I aim to change the perspective area light implementation into an orthographic directional light.
