#include "model.hlsli"

#define ADS_FLAG_DIFFUSE_ENABLED  1
#define ADS_FLAG_SPECULAR_ENABLED 2

cbuffer MaterialBuffer : register(b1)
{
    float3 cb_color;
    uint cb_adsPropertiesFlags;
    float3 cb_viewPos;
    float padding;
    float3 cb_lightDirection;
};

float3 createDiffuse(float3 normal, float3 lightDirection)
{
    if ((cb_adsPropertiesFlags & ADS_FLAG_DIFFUSE_ENABLED) == 0)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }

    float diff = saturate(dot(normal, lightDirection));
    float3 diffuse = diff * cb_color;
    return diffuse;
}

float3 createSpecular(float3 normal, float3 lightDirection, float3 worldPosition)
{
    if ((cb_adsPropertiesFlags & ADS_FLAG_SPECULAR_ENABLED) == 0)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }

    float3 viewDirection = normalize(cb_viewPos - worldPosition);
    float3 reflectDirection = reflect(-lightDirection, normal);
    float spec = pow(saturate(dot(viewDirection, reflectDirection)), 64.0f);
    float3 specular = 1.0 * spec * cb_color;
    return specular;
}

float4 main_ps(VSOut input) : SV_TARGET0
{
    float ambientStrength = 0.2;
    float3 ambient = ambientStrength * cb_color;

    float3 lightDirection = normalize(-cb_lightDirection);
    float3 diffuse =  createDiffuse(input.normal, lightDirection);
    float3 specular = createSpecular(input.normal, lightDirection, input.worldPosition);

    float3 output = ambient + diffuse + specular;

    return float4(output, 1.0f);
}
