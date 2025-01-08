# GLAD
set(GLAD_SRC_DIR "${THIRDPARTY_DIR}/glad/src/")
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
        "${THIRDPARTY_DIR}/glad/include"
)

# Dear ImGUI
if(WINDOW_PLATFORM_WIN32)
    set(IMGUI_WINDOW_IMPL imgui_impl_win32)
else()
    set(IMGUI_WINDOW_IMPL imgui_impl_glfw)
endif()
set(IMGUI_DIR "${THIRDPARTY_DIR}/imgui/")
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
    list(APPEND IMGUI_INCLUDE_DIRS "${glfw_SOURCE_DIR}/include")
endif()
target_include_directories(imgui
    SYSTEM
    PRIVATE
        "${IMGUI_INCLUDE_DIRS}"
)

