layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;

uniform mat4 u_model;
uniform mat4 u_mvp;
// Normal matrix is used for correctly transform the input vertex normals to
// for to world space. A normal matrix is the transpose of the
// inverse of the upper-left 3x3 portion of the model matrix.
//
// normalMatrix = mat3(transpose(inverse(modelMatrix)))
//
uniform mat3 u_normalMatrix;

out block
{
    vec3 fragPos;
    vec3 normal;
} Out;

void main()
{
    gl_Position = u_mvp * vec4(a_position, 1.0);
    Out.fragPos = mat3(u_model) * a_position;
    Out.normal = u_normalMatrix * a_normal;
}
