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

if(WIN32)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
endif()

include(FetchContent)

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

add_executable(${PROJECT_NAME} "")

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS"
    )
endif()

set(THIRDPARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty")

target_include_directories(${PROJECT_NAME}
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
        "${THIRDPARTY_DIR}"
        "${THIRDPARTY_DIR}/glad/include"
        "${THIRDPARTY_DIR}/imgui"
        "${THIRDPARTY_DIR}/imgui/backends"
)

add_subdirectory(src)
target_sources(${PROJECT_NAME}
    PRIVATE
    "${THIRDPARTY_DIR}/glad/src/gl.c"
    "${THIRDPARTY_DIR}/imgui/imgui.cpp"
    "${THIRDPARTY_DIR}/imgui/imgui_draw.cpp"
    "${THIRDPARTY_DIR}/imgui/imgui_tables.cpp"
    "${THIRDPARTY_DIR}/imgui/imgui_widgets.cpp"
    "${THIRDPARTY_DIR}/imgui/backends/imgui_impl_glfw.cpp"
    "${THIRDPARTY_DIR}/imgui/backends/imgui_impl_opengl3.cpp"
)

find_package(OpenGL REQUIRED)
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        assimp
        glm::glm
        glfw
        OpenGL::GL
)

add_custom_command(
    TARGET ${PROJECT_NAME}
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
)

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
# Linux releases are distributed as AppImage files for compatibility
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
