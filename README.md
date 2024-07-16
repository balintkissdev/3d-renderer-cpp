# C++ 3D renderer with OpenGL 4.3

A hardware accelerated 3D renderer written in C++, using OpenGL 4.3 as graphics API.

![Demo](doc/img/demo.png)

## Table of Contents

- [Download](#download)
- [Features](#features)
- [Requirements](#requirements)
- [Build Instructions](#build-instructions)
- [Usage](#usage)
- [Resources](#resources)

## Download

- [Windows 64-bit download](https://github.com/balintkissdev/3d-renderer/releases/download/0.0.0/3DRenderer-0.0.0-win64.zip)
- [Linux 64-bit download](https://github.com/balintkissdev/3d-renderer/releases/download/0.0.0/3DRenderer-0.0.0-linux-x86_64.AppImage)

## Features

- 3D model display from `OBJ` file format
- Fly-by FPS camera movement
- Skybox display using cube map
- Directional light with ADS (Ambient, Diffuse, Specular) lighting (Phong shading)
- Graphical user interface with Dear ImGui

## Requirements

This project requires an OpenGL 4.3 compatible graphics adapter to run. Check if your hardware supports OpenGL 4.3 and have the latest graphics driver installed.

- `std++20` compatible C++ compiler
- CMake 3.16 or newer

Required dependencies on Debian, Ubuntu, Linux Mint:

```sh
sudo apt install libwayland-dev libxkbcommon-dev xorg-dev
```

Required dependencies on Fedora, Red Hat:

```sh
sudo dnf install wayland-devel libxkbcommon-devel libXcursor-devel libXi-devel libXinerama-devel libXrandr-devel
```

All other dependencies are either included in `thirdparty` folder or automatically downloaded and built by `FetchContent` feature of CMake.

- [GLFW](glfw.org)
- [OpenGL Mathematics (GLM)](https://github.com/g-truc/glm)
- [Assimp](https://assimp.org/)
- [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h)
- [Dear ImGui](https://github.com/ocornut/imgui)

## Build

1. Clone the repository

```sh
git clone https://github.com/balintkissdev/3d-renderer-cpp.git
cd 3d-renderer-cpp
```

2. Create `build` directory and navigate into it

```sh
mkdir build
cd build
```

3. Configure project with CMake

```sh
cmake ..
```

4. Build the project

```sh
cmake --build . --config Release
```

5. Generate distributable package (ZIP file on Windows, AppImage on Linux)

```sh
cpack -C "Release"
```

## Usage

After building, run the executable in the `build` directory. On Windows:

```cmd
3DRenderer.exe
```

On Linux:

```cmd
./3DRenderer
```

Use keyboard and mouse to navigate the 3D environment.

- Movement: `W`, `A`, `S`, `D`
- Mouse look: `Right-click` and drag
- Ascend: `Spacebar`
- Descend: `Right CTRL`

Modify UI controls to change properties of the 3D model display.

## Resources

- *Utah Teapot* and *Stanford Bunny* model meshes are from [Stanford Computer Graphics Laboratory](https://graphics.stanford.edu/)
- Skybox texture images are from [learnopengl.com](https://learnopengl.com/Advanced-OpenGL/Cubemaps)