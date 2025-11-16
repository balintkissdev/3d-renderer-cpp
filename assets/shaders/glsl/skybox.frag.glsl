#if __VERSION__ <= 310
precision mediump float;
#endif

in vec3 v_uv;

uniform samplerCube u_skyboxTexture;

layout (location = 0) out vec4 o_fragColor;

void main()
{
    o_fragColor = texture(u_skyboxTexture, v_uv);
}
