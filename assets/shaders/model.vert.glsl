#version 430 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;

out vec3 FragPos;
out vec3 v_normal;

uniform mat4 model;
uniform mat4 mvp;
uniform mat3 normalMatrix;

void main()
{
    FragPos = vec3(model * vec4(position, 1.0));
    v_normal = normalMatrix * normal;
    gl_Position = mvp * vec4(position, 1.0);
}
