# Manage project source code organization, local download and build of
# dependencies, executable build customization and distributable packaging.
cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
set(PROJECT_NAME 3DRenderer)
project(${PROJECT_NAME})

set(THIRDPARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")

# Set default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' default.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# TODO: Automate from git tag
set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 2)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

option(WITH_GLFW "Use GLFW window implementation instead of native Win32 API (available only on Windows)" OFF)
if(WIN32)
    if(WITH_GLFW)
        set(WINDOW_PLATFORM_GLFW TRUE)
        set(WINDOW_PLATFORM_DEF "WINDOW_PLATFORM_GLFW")
        message(STATUS "Using GLFW window implementation on Windows")
    else()
        set(WINDOW_PLATFORM_WIN32 TRUE)
        set(WINDOW_PLATFORM_DEF "WINDOW_PLATFORM_WIN32")
        message(STATUS "Using native Win32 API window implementation")
    endif()
else()
    set(WINDOW_PLATFORM_GLFW TRUE)
    set(WINDOW_PLATFORM_DEF "WINDOW_PLATFORM_GLFW")
    message(STATUS "Using GLFW window implementation (required for non-Windows)")
endif()

option(WEBBUILD_CI "Specify if an Emscripten build is running in CI" OFF)

# Globally build without need to link to Visual C++ Runtime DLLs (needs Visual
# C++ Redistributable installed on user's system, use "dumpbin /dependents" in MSVC Developer
# Command Prompt to verify required DLLs to ship) not for just app but
# dependencies built by this project too.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)
# set(MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "" FORCE)

# Download and build dependencies without requiring the user to manually search
# the internet to download and copy libraries (Windows) or pollute the user's
# system with globally installed system packages (Linux).
include(FetchContent)

# Download GLFW for desktop build, otherwise skip this and use bundled
# Emscripten package
if(NOT EMSCRIPTEN AND WINDOW_PLATFORM_GLFW)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.4
    )
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(glfw)
endif()

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG v5.4.1
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)  # TODO: Bugprone because not scoped to Assimp build
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
# HACK: CI unable to find Emscripten zlib
if(WEBBUILD_CI)
    set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
endif()
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(assimp)
if(EMSCRIPTEN)
    # zlib implementation required to build Assimp with Emscripten
    set_target_properties(assimp PROPERTIES
        COMPILE_OPTIONS "--use-port=zlib"
        LINK_OPTIONS "--use-port=zlib"
    )
endif()

add_executable(${PROJECT_NAME} "")

# Set executable compiler flags
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
    WIN32_EXECUTABLE $<BOOL:${WIN32}>
)
# Treating compiler warnings as errors is turned off by default but can be
# enabled with `-DBUILD_WERROR=ON` during the CMake configuration. This is
# because the common practice in open-source projects is to **not** enable
# build warnigns as errors by default, avoiding situations where people with
# different build environments encounter warnings that prevent them from
# building without modifying the `CMakeLists.txt` file. Warnings as errors are
# enabled for automated CI builds.
option(BUILD_WERROR "Treat compiler warnings as errors")
target_compile_options(${PROJECT_NAME} PRIVATE
    # Disable exceptions
    # TODO: Find a way to disable exceptions on MSVC without breaking build
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-fno-exceptions>
    # Disable RTTI
    $<IF:$<CXX_COMPILER_ID:MSVC>,/GR-,-fno-rtti>
    # Enable all warnings
    $<IF:$<CXX_COMPILER_ID:MSVC>,/W4,-Wall -Wextra -Wpedantic>
    # Warnings as errors if enabled
    $<$<BOOL:${BUILD_WERROR}>:$<IF:$<CXX_COMPILER_ID:MSVC>,/WX,-Werror>>
)
target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN>
    "${WINDOW_PLATFORM_DEF}"
)

option(ENABLE_CLANG_TIDY "Enable static code analysis with clang-tidy during build")
if(ENABLE_CLANG_TIDY)
    find_program(
        CLANG_TIDY_EXE
        NAMES clang-tidy
    )
# FIXME: clang-tidy doesn't work together with Emscripten
    if(CLANG_TIDY_EXE AND NOT EMSCRIPTEN)
        message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
        execute_process(
            COMMAND ${CLANG_TIDY_EXE} --version
            OUTPUT_VARIABLE CLANG_TIDY_VERSION
        )
        message(STATUS "clang-tidy version: ${CLANG_TIDY_VERSION}")
        set(DO_CLANG_TIDY
            "${CLANG_TIDY_EXE}"
            "-p=${CMAKE_BINARY_DIR}"
            "--header-filter=^${CMAKE_SOURCE_DIR}/src/.*"
        )
        set_target_properties(${PROJECT_NAME} PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
    else()
        message(STATUS "clang-tidy not found.")
    endif()
else()
    unset(CLANG_TIDY_EXE CACHE)
    message(STATUS "clang-tidy is disabled.")
endif()

# Add sources to build
add_subdirectory(src)
add_subdirectory(${THIRDPARTY_DIR})

# Add header include folders
target_include_directories(${PROJECT_NAME}
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
# Make sure to use system includes to avoid clang-tidy checks on them
target_include_directories(${PROJECT_NAME}
    SYSTEM
    PRIVATE
        "${THIRDPARTY_DIR}"
        "${THIRDPARTY_DIR}/glad/include"
        "${THIRDPARTY_DIR}/imgui"
        "${THIRDPARTY_DIR}/imgui/backends"
        "${assimp_SOURCE_DIR}/include"
        "${glm_SOURCE_DIR}"
)
if(WINDOW_PLATFORM_GLFW)
    target_include_directories(${PROJECT_NAME}
        SYSTEM
        PRIVATE
            "${glfw_SOURCE_DIR}/include"
    )
endif()

# Link libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        assimp
        imgui
        glm::glm
        glad
        # "-sUSE_GLFW=3" linker option uses Emscripten GLFW package for WebAssembly
)
if(WINDOW_PLATFORM_GLFW)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            glfw
    )
endif()
if(NOT EMSCRIPTEN)
    find_package(OpenGL REQUIRED)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            OpenGL::GL
    )
endif()

# Copy assets
add_custom_command(
    TARGET ${PROJECT_NAME}
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
)

# Linker options for WebAssembly build
if(EMSCRIPTEN)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        SUFFIX ".html"
    )
    target_link_options(${PROJECT_NAME} PRIVATE
        "-sMIN_WEBGL_VERSION=2"
        "-sMAX_WEBGL_VERSION=2"
        # TODO: Consider switching to --use-port=contrib.glfw3 once fixed imgui bug
        "-sUSE_GLFW=3"
        "-sALLOW_MEMORY_GROWTH=1"
        "-sINITIAL_MEMORY=256MB"
        --preload-file "assets"
        --shell-file "${PROJECT_SOURCE_DIR}/src/shell_minimal.html"
    )
endif()

# Create distributable ZIP file for Windows releases
if(WIN32)
    set(CPACK_PACKAGE_NAME "3DRenderer")
    set(CPACK_PACKAGE_DESCRIPTION "Hardware 3D renderer")
    set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
    set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
    set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
    set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_README "${PROJECT_SOURCE_DIR}/README.md")
    set(CPACK_RESOURCE_FILE_THIRDPARTY_LICENSES "${PROJECT_SOURCE_DIR}/THIRD_PARTY_LICENSES.txt")

    install(FILES "${CPACK_RESOURCE_FILE_LICENSE}" "${CPACK_RESOURCE_FILE_README}" "${CPACK_RESOURCE_FILE_THIRDPARTY_LICENSES}" DESTINATION ".")
    install(TARGETS ${PROJECT_NAME} RUNTIME CONFIGURATIONS Release DESTINATION ".")
    install(DIRECTORY "${PROJECT_SOURCE_DIR}/assets/" DESTINATION "assets")
    set(CPACK_GENERATOR ZIP)
    include(CPack)
# Copy WebAssembly build on "make install" for easy web upload
elseif(EMSCRIPTEN)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.data" DESTINATION ".")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.js" DESTINATION ".")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.wasm" DESTINATION ".")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.html" RENAME "index.html" DESTINATION ".")
# Linux releases are distributed as AppImage files for compatibility with multiple distros
else()
    # Keep "assets" and "share" folder separate to avoid packaging icon as asset for the app
    set(ICON "${PROJECT_SOURCE_DIR}/share/icons/appimage.svg")
    install(CODE
    "include(${PROJECT_SOURCE_DIR}/cmake/modules/appimage.cmake)
    make_appimage(
        EXE \"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}\"
        NAME \"${PROJECT_NAME}\"
        ICON \"${ICON}\"
        DIR_ICON \"${ICON}\"
        OUTPUT_NAME \"${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-${PROJECT_VERSION}-linux-${CMAKE_SYSTEM_PROCESSOR}.AppImage\"
        ASSETS \"${CMAKE_CURRENT_BINARY_DIR}/assets\"
    )
    "
    COMPONENT Runtime
    )
endif()
