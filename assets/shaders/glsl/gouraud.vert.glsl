layout(location = 0) in vec3 a_vertexPosition;
layout(location = 1) in vec3 a_vertexNormal;

out vec3 v_rgb;

void main()
{
    gl_Position = u_mvp * vec4(a_vertexPosition, 1.0);
    vec3 worldPosition = mat3(u_model) * a_vertexPosition;
    vec3 worldNormal = convertToWorldNormal(a_vertexNormal);

    v_rgb = classicADSLightModel(worldPosition, worldNormal);
}
