#ifndef SKYBOX_H_
#define SKYBOX_H_

#ifdef __EMSCRIPTEN__
#include "glad/gles2.h"
#else
#include "glad/gl.h"
#endif

#include <memory>
#include <string>

class Skybox
{
public:
    friend class SkyboxBuilder;

    Skybox(const Skybox&) = delete;
    Skybox& operator=(const Skybox&) = delete;
    Skybox(Skybox&& other) noexcept;
    Skybox& operator=(Skybox&& other) noexcept;

    ~Skybox();

    GLuint textureID;
    GLuint vertexArray;

private:
    Skybox();

    GLuint vertexBuffer_;
    GLuint indexBuffer_;
};

class SkyboxBuilder
{
public:
    SkyboxBuilder& setRight(const std::string& rightFacePath);
    SkyboxBuilder& setLeft(const std::string& leftFacePath);
    SkyboxBuilder& setTop(const std::string& topFacePath);
    SkyboxBuilder& setBottom(const std::string& bottomFacePath);
    SkyboxBuilder& setFront(const std::string& frontFacePath);
    SkyboxBuilder& setBack(const std::string& backFacePath);
    std::unique_ptr<Skybox> build();

private:
    std::string rightFacePath_;
    std::string leftFacePath_;
    std::string topFacePath_;
    std::string bottomFacePath_;
    std::string frontFacePath_;
    std::string backFacePath_;
};

#endif
