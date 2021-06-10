# Adria-DX11

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/bf57622466be45c58d5a30735d989ad4)](https://app.codacy.com/gh/mate286/Adria-DX11?utm_source=github.com&utm_medium=referral&utm_content=mate286/Adria-DX11&utm_campaign=Badge_Grade_Settings)

Graphics engine written in C++/DirectX11.

## Features
* Entity-Component System
* Deferred + Forward Rendering 
* Tiled Deferred Rendering 
* Clustered Deferred Rendering
* Voxel Cone Tracing Global Illumination
* Physically Based Shading
* Image Based Lighting
* Normal Mapping
* Shadows
    - PCF Shadows for Directional, Spot and Point lights
    - Cascade Shadow Maps for Directional Lights
* Volumetric Lighting
    - Directional Lights with Shadow Maps
    - Directional Lights with Cascade Shadow Maps
    - Point and Spot Lights 
* HDR and Tone Mapping
* Bloom
* Depth Of Field + Bokeh  
    - Bokeh shapes supported - Hexagon, Octagon, Circle, Cross
* SSAO
* SSR
* FXAA
* TAA
* God Rays
* Lens Flare
* Fog
* Motion Blur
* Volumetric Clouds
* Ocean FFT
    - Adaptive Tesselation
    - Foam
* ImGui Editor
* Assimp Model Loading
* Profiler
* Camera and Light Frustum Culling

### To Do
* Atmospheric Scattering
* Terrain
* Particle Engine

## Dependencies
[assimp](https://github.com/assimp/assimp)

[ImGui](https://github.com/ocornut/imgui)

[ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo)

[ImGuiFileDialog](https://github.com/aiekick/ImGuiFileDialog)

[stb](https://github.com/nothings/stb)

## Screenshots

Editor 
![alt text](Screenshots/editor.png "Editor")

<table>
  <tr>
    <td>Tiled Deferred Rendering with 256 lights</td>
     <td>Tiled Deferred Rendering Visualized</td>
     </tr>
  <tr>
    <td><img src="Screenshots/tiled.png"></td>
    <td><img src="Screenshots/tiled_visualization.png"></td>
  </tr>
 </table>
 
 <table>
  <tr>
     <td>Voxel Cone Tracing Global Illumination</td>
     <td>Voxelized Scene</td>
     </tr>
  <tr>
    <td><img src="Screenshots/gi.png"></td>
    <td><img src="Screenshots/gi_debug.png"></td>
  </tr>
 </table>
 
 Image Based Lighting 
![alt text](Screenshots/ibl.png "Image Based Lighting ")
 
Ocean and Lens Flare
![alt text](Screenshots/ocean_lens_flare.png "Ocean and Lens Flare")
 
 Volumetric Lighting
![alt text](Screenshots/volumetric_dir.png " Volumetric Directional Lighting")
![alt text](Screenshots/volumetric_point.png " Volumetric Point Lighting")

Bokeh
![alt text](Screenshots/bokeh.png "Bokeh")

Volumetric Clouds and God Rays
![alt text](Screenshots/clouds.gif "Clouds")




