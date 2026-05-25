in vec3 v_rgb;

layout(location = 0) out vec4 o_FragColor;

void main()
{
    o_FragColor = vec4(v_rgb, 1.0);
}
