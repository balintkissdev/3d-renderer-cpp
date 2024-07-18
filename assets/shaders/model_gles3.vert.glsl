#version 300 es
precision mediump float;

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;

uniform mat4 u_model;
uniform mat4 u_mvp;
uniform mat3 u_normalMatrix;

out vec3 v_fragPos;
out vec3 v_normal;

void main()
{
    gl_Position = u_mvp * vec4(a_position, 1.0);
    v_fragPos = vec3(u_model * vec4(a_position, 1.0));
    v_normal = u_normalMatrix * a_normal;
}
