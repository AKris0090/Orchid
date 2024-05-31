# ORCHID

## My goals for this project
* Create an engine I can use as a base for a third person shooter game demo
* Have a space I can use to try to implement various graphics programming concepts
* Create a portfolio piece I can use to both demonstrate my game engine and graphics programming knowledge

## Current Features
### PBR Textures
|                                                     Base Color                                                  |                                                     Normal                                                                    |
|                                      ![](README_IMAGES/pbr/basecolor.png)                                       |                                          ![](README_IMAGES/pbr/normal.png)                                                    |
|                                                     Metallic Roughness                                          |                                                     Final Output                                                              |
|                                      ![](README_IMAGES/pbr/metallicroughness.png)                               |                                          ![](README_IMAGES/pbr/combined.png)                                                  |

I implemented the Cook-Torrence BRDF with reference from: https://learnopengl.com/PBR/Theory. When loading a gltf file, the gltf loader searches for each texture (albedo, normal, metallic/roughness, ao, and emission), and if it cannot find any, a default texture is used. 
