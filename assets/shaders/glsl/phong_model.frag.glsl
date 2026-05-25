in vec3 v_fragPos;
in vec3 v_normal;

layout(location = 0) out vec4 o_FragColor;

void main()
{
    vec3 ambient = calculateAmbient();

    // Keep normalize() for normal vector. Interpolation between shader stages
    // might change the normal vector length and it won't be a unit vector anymore,
    // resulting in wrong results.
    vec3 tnorm = normalize(v_normal);
    vec3 lightDirection = normalize(-u_light.direction);

    vec3 diffuse = calculateDiffuse(tnorm, lightDirection);
    vec3 specular = calculateSpecular(tnorm, lightDirection, v_fragPos);
    vec3 rgb = ambient + diffuse + specular;

    o_FragColor = vec4(rgb, 1.0);
}
