
<img align="left" src="Adria/Resources/Icons/adria_logo.jpg" width="120px"/>

# Adria-DX11
Graphics engine written in C++/DirectX11. For successful build you will need textures that you can find [here](https://github.com/mateeeeeee/Adria-DX11/releases/tag/1.0).

## Features
* Deferred + forward rendering 
* Tiled deferred rendering 
* Clustered deferred rendering
* Physically based shading
* Image based lighting
* Normal mapping
* Shadows
    - PCF Shadows for directional, spot and point lights
    - Cascade shadow maps for directional lights
* Volumetric Lighting
    - Directional lights with shadow maps
    - Directional lights with cascade shadow maps
    - Point and Spot Lights 
* HDR and Tone Mapping
* Bloom
* Depth Of Field + Bokeh  
    - Bokeh shapes supported - Hexagon, Octagon, Circle, Cross
* Ambient Occlusion: SSAO, HBAO
* Film effects: Lens distortion, Chromatic aberration, Vignette, Film grain
* SSR
* SSCS
* Deferred decals
* FXAA
* TAA
* God rays
* Lens flare
* Fog
* Motion blur
* Volumetric clouds
* Ocean FFT
    - Adaptive tesselation
    - Foam
* Procedural terrain with instanced foliage and trees
* Hosek-Wilkie sky model
* Particles
* ImGui editor
* Model loading with tinygltf
* Profiler
* Shader hot reloading
* ECS


## Dependencies
[tinygltf](https://github.com/syoyo/tinygltf)

[ImGui](https://github.com/ocornut/imgui)

[ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo)

[ImGuiFileDialog](https://github.com/aiekick/ImGuiFileDialog)

[stb](https://github.com/nothings/stb)

[FastNoiseLite](https://github.com/Auburn/FastNoiseLite)

[json](https://github.com/nlohmann/json)

## Screenshots

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
 
 Film Effects
![alt text](Screenshots/film.png "Film Effects")
 
Ocean and Lens Flare
![alt text](Screenshots/ocean_lens_flare.png "Ocean and Lens Flare")
 
 Volumetric Lighting
![alt text](Screenshots/volumetric_dir.png " Volumetric Directional Lighting")
![alt text](Screenshots/volumetric_point.png " Volumetric Point Lighting")

Bokeh
![alt text](Screenshots/bokeh.png "Bokeh")

Volumetric Clouds
![alt text](Screenshots/clouds.png "Clouds")

Hosek-Wilkie Sky Model
![alt text](Screenshots/hosek_wilkie.png "Hosek-Wilkie")

God Rays and Instanced Foliage
![alt text](Screenshots/foliage.png "God Rays and Instanced Foliage")

Deferred Decals
![alt text](Screenshots/decals.png "Deferred Decals")


