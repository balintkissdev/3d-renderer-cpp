cbuffer MaterialBuffer : register(b1)
{
    float3 cb_color;
    float cb_specularReflectivity;
    float3 cb_viewPos;
    float cb_shininess;
    // TODO: To support multiple lights, lights will have their own separate CB
    float3 cb_lightDirection;
};

float3 convertToWorldNormal(float3 normal, float4x4 normalMatrix)
{
    return mul(normal, (float3x3)normalMatrix);
}

float calculateCosineTerm(float3 a, float3 b)
{
    return saturate(dot(a, b));
}

float3 calculateAmbient()
{
    float ambientStrength = 0.2;
    return ambientStrength * cb_color;
}

float3 calculateSpecular(float3 worldPosition, float3 surfaceNormal, float3 lightDirection)
{
    float3 viewDirection = normalize(cb_viewPos - worldPosition);
#ifdef BLINN_PHONG
    // Is: Ls * Ks * dot(h, n)
    // h (halfway vector): v + s
    float3 halfWayVector = normalize(viewDirection + lightDirection);
    float normalizedShininess = cb_shininess * 4.0; // General rule is between 2-4. Results will still be different.
    float specularIntensity = pow(calculateCosineTerm(halfWayVector, surfaceNormal), normalizedShininess);
#else
    // reflect(I,N) = I - 2.0 * dot(N, I) * N
    // I: Incident vector
    // N: Normal vector
    float3 reflectDirection = reflect(-lightDirection, surfaceNormal);
    float specularIntensity = pow(calculateCosineTerm(reflectDirection, viewDirection), cb_shininess);
#endif
    return cb_specularReflectivity * cb_color * specularIntensity;
}

// ADS lighting equation is
// I = Ia + Id + Is
// L: light source intensity
// K: surface material reflectivity
// Ia (ambient): La * Ka
// Id (diffuse): Ld * Kd * dot(s, n)
//   s: light source
//   n: surface point normal vector
// Is (specular): Ls * Ks * dot(r, v)^f
//   r: pure reflection, r = -s + 2 * dot(s, n) * n
//   v: camera view position
//   f: specular highlight Shininess exponent
float3 classicADSLightModel(float3 worldPosition, float3 worldNormal)
{
    // Keep normalize() for normal vector. Interpolation between shader stages
    // might change the normal vector length and it won't be a unit vector anymore,
    // resulting in wrong results.
    float3 tnorm = normalize(worldNormal);
    float3 lightDirection = normalize(-cb_lightDirection);

    float3 ambient = calculateAmbient();
    float sDotN = calculateCosineTerm(lightDirection, tnorm);
    float3 diffuse = sDotN * cb_color;
    float3 specular;
    if (0.0 < sDotN)    // TODO: Optimize
    {
        specular = calculateSpecular(worldPosition, tnorm, lightDirection);
    }
    else
    {
        specular = float3(0.0, 0.0, 0.0);
    }

    return ambient + diffuse + specular;
}

