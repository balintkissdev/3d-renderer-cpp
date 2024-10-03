# Real-time C++ 3D renderer with OpenGL 4 and WebGL2

[![Build](https://github.com/balintkissdev/3d-renderer-cpp/actions/workflows/main.yml/badge.svg)](https://github.com/balintkissdev/3d-renderer-cpp/actions/workflows/main.yml)

> A hardware-accelerated 3D renderer written in C++. Runs using OpenGL 4.3 as
graphics API on desktop and WebGL2 in web browsers.

[Click here for Rust version of this project](https://github.com/balintkissdev/3d-renderer-rust)

![Demo](doc/img/demo.png)

## Table of Contents

- [Try it out!](#try-it-out)
- [Motivation](#motivation)
- [Features](#features)
- [Requirements](#requirements)
- [Build Instructions](#build-instructions)
- [Usage](#usage)
- [Resources](#resources)

## Try it out!

- [Live demo in web browser](https://www.balintkissdev.com/3d-renderer-cpp)
- [Windows 64-bit download](https://github.com/balintkissdev/3d-renderer-cpp/releases/download/0.0.4/3DRenderer-0.0.4-win64.zip)
- [Linux 64-bit download](https://github.com/balintkissdev/3d-renderer-cpp/releases/download/0.0.4/3DRenderer-0.0.4-linux-x86_64.AppImage)

## Motivation

This project is a demonstration of my ability to write cross-platform 3D
graphical applications in C++ that run on both desktop (Windows, Linux) and on
the web with WebAssembly. I designed my application to balance the correct
level of abstractions and performance optimizations. The project showcases
confident usage of the following technologies:

- C++20
- 3D graphics programming with OpenGL 4.x, WebGL2 (based on OpenGL ES 3.0)
- Advanced CMake practices (modern CMake targets, FetchContent, CPack)
- Immediate mode overlay GUI using Dear ImGui (as opposed to retained mode GUI frameworks like Qt)
- Building for WebAssembly using Emscripten
- Clang Tooling (clang-format, clang-tidy)

I plan to use this project as reference when I do native game engine development projects at home.

Future additions will include Direct3D, Vulkan rendering backends and additional post-processing effects.

## Features

- 3D model display from `OBJ` file format
- Fly-by FPS camera movement
- Skybox display using cube-map
- Directional light with ADS (Ambient, Diffuse, Specular) lighting (Phong shading)
- Live browser demo

## Requirements

Desktop executable requires an OpenGL 4.3 compatible graphics adapter to run.
Check if your hardware supports OpenGL 4.3 and have the latest graphics driver
installed.

Web browser live demo requires support for WebGL2.

- C++20 compiler
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

- [Assimp](https://assimp.org/)
- [Dear ImGui](https://github.com/ocornut/imgui)
- [GLFW](glfw.org)
- [OpenGL Mathematics (GLM)](https://github.com/g-truc/glm)
- [glad](https://gen.glad.sh/)
- [stb_image](https://github.com/nothings/stb/blob/master/stb_image.h)

## Build instructions

1. Clone the repository

```sh
git clone https://github.com/balintkissdev/3d-renderer-cpp.git
cd 3d-renderer-cpp
```

2. Configure the project with CMake. `-B build` also creates a new folder
   called `build` where the files during build will be generated, avoiding
   polluting the project root folder.

```sh
cmake -B ./build
```

Treating compiler warnings as errors is turned off by default
but can be enabled with `-DBUILD_WERROR=ON` during the CMake configuration.
This is because the common practice in open-source projects is to **not** enable
build warnigns as errors by default, avoiding situations where people with
different build environments encounter warnings that prevent them from building
without modifying the `CMakeLists.txt` file. Warnings as errors are enabled for
automated CI builds.

3. Build the project

```sh
cmake --build ./build --config Release
```

### Generate Windows release

This will be a ZIP file on Windows.

```sh
cpack -B ./build -C "Release"
```

### Generate Linux release

This will be an AppImage file on Linux. Generation requires FUSE version 2 to
be installed (see https://github.com/AppImage/AppImageKit/wiki/FUSE).

```
cd build
make install DESTDIR=.
```

### WebAssembly build

Install
[Emscripten](https://emscripten.org/docs/getting_started/downloads.html) on
your system then issue the commands for configure and build similar as before,
but in the Emscripten build environment.

```sh
source <emsdk install location>/emsdk_env.sh

emcmake cmake -B ./build
emcmake cmake --build ./build --config Release
```

Opening the resulting `3Drenderer.html` file with the browser will not work because of
default browser CORS rules. You can either use a live server locally to access
at `http://localhost:8000`

```sh
cd build
python -m http.server
```

or alternatively use `emrun 3DRenderer.html --port 8000`.

## Usage

After building, run the executable in the `build` directory. On Windows:

```cmd
cd build
3DRenderer.exe
```

On Linux:

```cmd
cd build
./3DRenderer
```

Use keyboard and mouse to navigate the 3D environment.

- Movement: `W`, `A`, `S`, `D`
- Mouse look: `Right-click` and drag
- Ascend: `Spacebar`
- Descend: `C`

Modify UI controls to change properties of the 3D model display.

## Resources

- *Utah Teapot* and *Stanford Bunny* model meshes are from [Stanford Computer Graphics Laboratory](https://graphics.stanford.edu/)
    - High poly *Stanford Bunny* model mesh is from https://www.prinmath.com/csci5229/OBJ/index.html
- Skybox texture images are from [learnopengl.com](https://learnopengl.com/Advanced-OpenGL/Cubemaps)
