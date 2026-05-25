in vec3 v_worldPosition;
in vec3 v_worldNormal;

layout(location = 0) out vec4 o_FragColor;

void main()
{
    vec3 rgb = classicADSLightModel(v_worldPosition, v_worldNormal);
    o_FragColor = vec4(rgb, 1.0);
}
