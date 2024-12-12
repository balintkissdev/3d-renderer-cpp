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
add_library(imgui
    STATIC
        "${THIRDPARTY_DIR}/imgui/imgui.cpp"
        "${THIRDPARTY_DIR}/imgui/imgui_draw.cpp"
        "${THIRDPARTY_DIR}/imgui/imgui_tables.cpp"
        "${THIRDPARTY_DIR}/imgui/imgui_widgets.cpp"
        "${THIRDPARTY_DIR}/imgui/backends/${IMGUI_WINDOW_IMPL}.cpp"
        "${THIRDPARTY_DIR}/imgui/backends/imgui_impl_opengl3.cpp"
)

set(IMGUI_INCLUDE_DIRS "${THIRDPARTY_DIR}/imgui")
if(NOT EMSCRIPTEN)
    list(APPEND IMGUI_INCLUDE_DIRS "${glfw_SOURCE_DIR}/include")
endif()
target_include_directories(imgui
    SYSTEM
    PRIVATE
        "${IMGUI_INCLUDE_DIRS}"
)

