#version 300 es
precision mediump float;

in vec3 v_texCoords;

uniform samplerCube u_skyboxTexture;

layout (location = 0) out vec4 v_fragColor;

void main()
{
    v_fragColor = texture(u_skyboxTexture, v_texCoords);
}
