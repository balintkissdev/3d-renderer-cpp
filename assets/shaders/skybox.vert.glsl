#version 430 core

layout (location = 0) in vec3 a_position;

uniform mat4 u_projectionView;

out vec3 v_texCoords;

void main()
{
    vec4 pos = u_projectionView * vec4(a_position, 1.0);
    gl_Position = pos.xyww;
    v_texCoords = a_position;
}
