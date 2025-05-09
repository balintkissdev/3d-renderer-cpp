if(WINDOW_PLATFORM_WIN32)
    set(WINDOW_PLATFORM_PREFIX win32)
    target_sources(${PROJECT_NAME}
        PRIVATE
            direct3d12/d3d12_model.cpp
            direct3d12/d3d12_model.hpp
            direct3d12/d3d12_renderer.cpp
            direct3d12/d3d12_renderer.hpp
            gl/wgl_context.cpp
            gl/wgl_context.hpp
    )
else()
    set(WINDOW_PLATFORM_PREFIX glfw)
endif()

target_sources(${PROJECT_NAME}
    PRIVATE
        app.cpp
        app.hpp
        camera.cpp
        camera.hpp
        drawproperties.cpp
        drawproperties.hpp
        gl/gl_model.cpp
        gl/gl_model.hpp
        gl/gl_renderer.cpp
        gl/gl_renderer.hpp
        gl/gl_shader.cpp
        gl/gl_shader.hpp
        gl/gl_skybox.cpp
        gl/gl_skybox.hpp
        gui.cpp
        gui.hpp
        main.cpp
        modelimporter.cpp
        modelimporter.hpp
        renderer.cpp
        renderer.hpp
        scene.cpp
        scene.hpp
        utils.hpp
        window.hpp
        "${WINDOW_PLATFORM_PREFIX}_window.cpp"
        "${WINDOW_PLATFORM_PREFIX}_window.hpp"
)

