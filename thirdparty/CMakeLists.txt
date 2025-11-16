# GLFW
#
# Use GLFW for desktop build, otherwise skip this and use bundled
# Emscripten package
if(NOT EMSCRIPTEN AND WINDOW_PLATFORM_GLFW)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory("${THIRDPARTY_DIR}glfw")
endif()


# GLAD
set(GLAD_SRC_DIR "${THIRDPARTY_DIR}glad/src/")
if(EMSCRIPTEN)
    set(GLAD_SOURCES "${GLAD_SRC_DIR}gles2.c")
else()
    set(GLAD_SOURCES "${GLAD_SRC_DIR}gl.c")
    if(WIN32)
        list(APPEND GLAD_SOURCES "${GLAD_SRC_DIR}wgl.c")
    endif()
endif()
add_library(glad "${GLAD_SOURCES}")

target_include_directories(glad
    SYSTEM
    PRIVATE
        "${THIRDPARTY_DIR}glad/include"
)

# GLM
add_subdirectory("${THIRDPARTY_DIR}glm")

# Assimp
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
add_subdirectory("${THIRDPARTY_DIR}assimp")
if(EMSCRIPTEN)
    # zlib implementation required to build Assimp with Emscripten
    set_target_properties(assimp PROPERTIES
        COMPILE_OPTIONS "--use-port=zlib"
        LINK_OPTIONS "--use-port=zlib"
    )
endif()

# Dear ImGUI
if(WINDOW_PLATFORM_WIN32)
    set(IMGUI_WINDOW_IMPL imgui_impl_win32)
else()
    set(IMGUI_WINDOW_IMPL imgui_impl_glfw)
endif()
set(IMGUI_DIR "${THIRDPARTY_DIR}imgui/")
set(IMGUI_SOURCES
    "${IMGUI_DIR}imgui.cpp"
    "${IMGUI_DIR}imgui_draw.cpp"
    "${IMGUI_DIR}imgui_tables.cpp"
    "${IMGUI_DIR}imgui_widgets.cpp"
    "${IMGUI_DIR}backends/${IMGUI_WINDOW_IMPL}.cpp"
    "${IMGUI_DIR}backends/imgui_impl_opengl3.cpp"
)
if(WINDOW_PLATFORM_WIN32)
    list(APPEND IMGUI_SOURCES "${IMGUI_DIR}backends/imgui_impl_dx12.cpp")
endif()
add_library(imgui STATIC ${IMGUI_SOURCES})
set(IMGUI_INCLUDE_DIRS "${IMGUI_DIR}")
if(NOT EMSCRIPTEN)
    list(APPEND IMGUI_INCLUDE_DIRS "${THIRDPARTY_DIR}glfw/include")
endif()
target_include_directories(imgui
    SYSTEM
    PRIVATE
        "${IMGUI_INCLUDE_DIRS}"
)

# DirectXTex
if(WIN32)
    set(BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(BUILD_SAMPLE OFF CACHE BOOL "" FORCE)
    set(BC_USE_OPENMP OFF CACHE BOOL "" FORCE)
    add_subdirectory("${THIRDPARTY_DIR}DirectXTex")
endif()
