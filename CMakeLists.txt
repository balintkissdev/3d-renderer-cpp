cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
set(PROJECT_NAME 3DRenderer)
project(${PROJECT_NAME})

# Set default build type to Release
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' default.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

set(PROJECT_VERSION_MAJOR 0)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)
set(PROJECT_VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

# Global flags for not just app but dependencies too
if(MSVC)
    # Build without MSVCRT.dll
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

include(FetchContent)

# Download GLFW for desktop build, otherwise skip this and use bundled
# Emscripten package
if(NOT EMSCRIPTEN)
    FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.3.10
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
set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(assimp)
if(EMSCRIPTEN)
    # zlib implementation required by Assimp
    set_target_properties(assimp PROPERTIES
        COMPILE_OPTIONS "-sUSE_ZLIB=1"
        LINK_OPTIONS "-sUSE_ZLIB=1"
    )
endif()

add_executable(${PROJECT_NAME} "")

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        # TODO: Find a way to disable exceptions on MSVC without breaking build
        /GR-                  # Disable RTTI
        /W3                   # Warning level 3
        /WX                   # Treat all warnings as errors
    )
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS /SUBSYSTEM:WINDOWS
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        -fno-exceptions        # Disable exceptions
        -fno-rtti              # Disable RTTI
        -Wall                  # Enable all warnings
        -Wextra                # Enable extra warnings
        -Werror                # Treat all warnings as errors
        -Wpedantic             # Enable pedantic warnings
    )
endif()

find_program(
    CLANG_TIDY_EXE
    NAMES clang-tidy
)
# FIXME: clang-tidy doesn't work together with Emscripten
if(CLANG_TIDY_EXE AND NOT EMSCRIPTEN)
    message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
    set(DO_CLANG_TIDY
        "${CLANG_TIDY_EXE}"
        "--header-filter=src/*"
    )
    set_target_properties(${PROJECT_NAME} PROPERTIES CXX_CLANG_TIDY "${DO_CLANG_TIDY}")
else()
    message(STATUS "clang-tidy not found.")
endif()

set(THIRDPARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")

target_include_directories(${PROJECT_NAME}
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
# TODO: Cleanup to remove duplication
if(EMSCRIPTEN)
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
else()
    target_include_directories(${PROJECT_NAME}
        SYSTEM
        PRIVATE
            "${THIRDPARTY_DIR}"
            "${THIRDPARTY_DIR}/glad/include"
            "${THIRDPARTY_DIR}/imgui"
            "${THIRDPARTY_DIR}/imgui/backends"
            "${assimp_SOURCE_DIR}/include"
            "${glfw_SOURCE_DIR}/include"
            "${glm_SOURCE_DIR}"
    )
endif()
add_subdirectory(src)
add_subdirectory(${THIRDPARTY_DIR})

if(EMSCRIPTEN)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            assimp
            imgui
            glm::glm
            glad
    )
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            assimp
            imgui
            glm::glm
            glfw
            glad
            OpenGL::GL
    )
endif()

add_custom_command(
    TARGET ${PROJECT_NAME}
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
)

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
