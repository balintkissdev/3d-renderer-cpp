#version 430 core

layout (location = 0) in vec3 position;

uniform mat4 projectionView;

out vec3 TexCoords;

void main()
{
    TexCoords = position;
    vec4 pos = projectionView * vec4(position, 1.0);
    gl_Position = pos.xyww;
}
