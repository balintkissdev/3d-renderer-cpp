# C++ 3D renderer with OpenGL 4.3 and OpenGL ES 3.0

A hardware accelerated 3D renderer written in C++. Runs using OpenGL 4.3  as
graphics API on desktop and with OpenGL ES 3.0 in web browsers.

![Demo](doc/img/demo.png)

## Table of Contents

- [Try it out!](#try-it-out)
- [Features](#features)
- [Requirements](#requirements)
- [Build Instructions](#build-instructions)
- [Usage](#usage)
- [Resources](#resources)

## Try it out!

- [Live demo in web browser](https://www.balintkissdev.com/3d-renderer-cpp)
- [Windows 64-bit download](https://github.com/balintkissdev/3d-renderer-cpp/releases/download/0.0.0/3DRenderer-0.0.0-win64.zip)
- [Linux 64-bit download](https://github.com/balintkissdev/3d-renderer-cpp/releases/download/0.0.0/3DRenderer-0.0.0-linux-x86_64.AppImage)

## Features

- 3D model display from `OBJ` file format
- Fly-by FPS camera movement
- Skybox display using cube map
- Directional light with ADS (Ambient, Diffuse, Specular) lighting (Phong shading)
- Graphical user interface with immediate mode GUI using Dear ImGui
- Live browser demo as WebAssembly by Emscripten compiler

## Requirements

Desktop executable requires an OpenGL 4.3 compatible graphics adapter to run.
Check if your hardware supports OpenGL 4.3 and have the latest graphics driver
installed.

Web browser live demo requires support for OpenGL ES 3.0 used by WebGL2.

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

### Generate distributable package

This will be a ZIP file on Windows and AppImage on Linux.

```sh
cpack -C "Release"
```

### WebAssembly build

Install
[Emscripten](https://emscripten.org/docs/getting_started/downloads.html) on
your system then issue the commands for configure and build similar as before,
but in the Emscripten build environment.

```sh
source <emsdk install location>/emsdk_env.sh

cd build
emcmake cmake ..
emmake make
```

Opening the resulting `3Drenderer.html` file with the browser will not work because of
default browser CORS rules. You can either use a live server locally to access
at `http://localhost:8000`

```sh
python -m http.server
```

or alternatively use `emrun 3DRenderer.html`.

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
- Descend: `C`

Modify UI controls to change properties of the 3D model display.

## Resources

- *Utah Teapot* and *Stanford Bunny* model meshes are from [Stanford Computer Graphics Laboratory](https://graphics.stanford.edu/)
- Skybox texture images are from [learnopengl.com](https://learnopengl.com/Advanced-OpenGL/Cubemaps)
