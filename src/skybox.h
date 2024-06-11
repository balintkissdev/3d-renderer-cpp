#pragma once

#include "shader.h"

#include "glad/gl.h"
#include "glm/mat4x4.hpp"

#include <memory>
#include <string>

class Camera;

class Skybox
{
public:
    friend class SkyboxBuilder;

    ~Skybox();
    void draw(const glm::mat4 &projection, const Camera &camera);

private:
    GLuint textureID_;
    GLuint vertexArray_;
    GLuint vertexBuffer_;
    GLuint indexBuffer_;
    std::unique_ptr<Shader> shader_;
};

class SkyboxBuilder
{
public:
    SkyboxBuilder& setRight(const std::string &rightFacePath);
    SkyboxBuilder& setLeft(const std::string &leftFacePath);
    SkyboxBuilder& setTop(const std::string &topFacePath);
    SkyboxBuilder& setBottom(const std::string &bottomFacePath);
    SkyboxBuilder& setFront(const std::string &frontFacePath);
    SkyboxBuilder& setBack(const std::string &backFacePath);
    std::unique_ptr<Skybox> build();

private:
    std::string rightFacePath_;
    std::string leftFacePath_;
    std::string topFacePath_;
    std::string bottomFacePath_;
    std::string frontFacePath_;
    std::string backFacePath_;
};
