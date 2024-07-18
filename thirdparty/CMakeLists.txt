if(EMSCRIPTEN)
    add_library(glad "${THIRDPARTY_DIR}/glad/src/gles2.c")
else()
    add_library(glad "${THIRDPARTY_DIR}/glad/src/gl.c")
endif()
target_include_directories(glad
    SYSTEM PRIVATE "${THIRDPARTY_DIR}/glad/include"
)

add_library(imgui
    "${THIRDPARTY_DIR}/imgui/imgui.cpp"
    "${THIRDPARTY_DIR}/imgui/imgui_draw.cpp"
    "${THIRDPARTY_DIR}/imgui/imgui_tables.cpp"
    "${THIRDPARTY_DIR}/imgui/imgui_widgets.cpp"
    "${THIRDPARTY_DIR}/imgui/backends/imgui_impl_glfw.cpp"
    "${THIRDPARTY_DIR}/imgui/backends/imgui_impl_opengl3.cpp"
)
if(EMSCRIPTEN)
target_include_directories(imgui
    SYSTEM
    PRIVATE
        "${THIRDPARTY_DIR}/imgui"
)
else()
target_include_directories(imgui
    SYSTEM
    PRIVATE
        "${THIRDPARTY_DIR}/imgui"
        "${glfw_SOURCE_DIR}/include"
)
endif()
