layout(location = 0) in vec3 a_vertexPosition;
layout(location = 1) in vec3 a_vertexNormal;

out vec3 v_worldPosition;
out vec3 v_worldNormal;

void main()
{
    gl_Position = u_mvp * vec4(a_vertexPosition, 1.0);
    v_worldPosition = mat3(u_model) * a_vertexPosition;
    v_worldNormal = convertToWorldNormal(a_vertexNormal);
}
