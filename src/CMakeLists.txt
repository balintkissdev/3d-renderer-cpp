if(WINDOW_PLATFORM_WIN32)
    set(WINDOW_PLATFORM_PREFIX win32)
    target_sources(${PROJECT_NAME}
        PRIVATE
            wgl_context.cpp
            wgl_context.hpp
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
        gui.cpp
        gui.hpp
        main.cpp
        model.cpp
        model.hpp
        renderer.cpp
        renderer.hpp
        scene.cpp
        scene.hpp
        shader.cpp
        shader.hpp
        skybox.cpp
        skybox.hpp
        utils.hpp
        "${WINDOW_PLATFORM_PREFIX}_window.cpp"
        "${WINDOW_PLATFORM_PREFIX}_window.hpp"
        window.hpp
)

