#version 430 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;

uniform mat4 u_model;
uniform mat4 u_mvp;
// Normal matrix is used for correctly transform the input vertex normals to
// for to world space. A normal matrix is the transpose of the
// inverse of the upper-left 3x3 portion of the model matrix.
//
// normalMatrix = mat3(transpose(inverse(modelMatrix)))
//
uniform mat3 u_normalMatrix;

out vec3 v_fragPos;
out vec3 v_normal;

void main()
{
    gl_Position = u_mvp * vec4(a_position, 1.0);
    v_fragPos = vec3(u_model * vec4(a_position, 1.0));
    v_normal = u_normalMatrix * a_normal;
}
