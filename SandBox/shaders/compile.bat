C:/VulkanSDK/1.3.268.0/Bin/glslc.exe glsl/shader.vert -o spv/vert.spv -g
C:/VulkanSDK/1.3.268.0/Bin/glslc.exe glsl/shader.frag -o spv/frag.spv -g
C:/VulkanSDK/1.3.268.0/Bin/glslc.exe glsl/outline.vert -o spv/outlineVert.spv -g
C:/VulkanSDK/1.3.268.0/Bin/glslc.exe glsl/outline.frag -o spv/outlineFrag.spv -g
C:/VulkanSDK/1.3.268.0/Bin/glslc.exe glsl/persona.frag -o spv/toonFrag.spv -g

C:/VulkanSDK/1.3.268.0/Bin/glslc.exe glsl/shadowMap.vert -o spv/shadowMap.spv -g
C:/VulkanSDK/1.3.268.0/Bin/glslc.exe glsl/depthPrePass.vert -o spv/depthPass.spv -g
C:/VulkanSDK/1.3.268.0/Bin/glslc.exe glsl/depthPrePassFrag.frag -o spv/depthPassAlpha.spv -g

C:/VulkanSDK/1.3.268.0/Bin/glslc.exe glsl/brdfLUT.vert -o spv/brdfLUTVert.spv -g
C:/VulkanSDK/1.3.268.0/Bin/glslc.exe glsl/brdfLUT.frag -o spv/brdfLUTFrag.spv -g
C:/VulkanSDK/1.3.268.0/Bin/glslc.exe glsl/filterCube.vert -o spv/filterCubeVert.spv -g
C:/VulkanSDK/1.3.268.0/Bin/glslc.exe glsl/irradianceCube.frag -o spv/irradianceCubeFrag.spv -g
C:/VulkanSDK/1.3.268.0/Bin/glslc.exe glsl/prefEnvMap.frag -o spv/prefilteredEnvMapFrag.spv -g
C:/VulkanSDK/1.3.268.0/Bin/glslc.exe glsl/skybox.vert -o spv/skyboxVert.spv -g
C:/VulkanSDK/1.3.268.0/Bin/glslc.exe glsl/skybox.frag -o spv/skyboxFrag.spv -g

C:/VulkanSDK/1.3.268.0/Bin/glslc.exe glsl/screenQuad.vert -o spv/screenQuadVert.spv  -g
C:/VulkanSDK/1.3.268.0/Bin/glslc.exe glsl/tonemapping.frag -o spv/tonemappingFrag.spv -g

C:/VulkanSDK/1.3.268.0/Bin/glslc.exe glsl/downSample.frag -o spv/downSample.spv -g
C:/VulkanSDK/1.3.268.0/Bin/glslc.exe glsl/upSample.frag -o spv/upSample.spv -g

C:/VulkanSDK/1.3.268.0/Bin/glslc.exe glsl/skinning.comp -o spv/computeSkin.spv -g
C:/VulkanSDK/1.3.268.0/Bin/glslc.exe glsl/frustrumCull.comp -o spv/frustrumCull.spv -g

pause