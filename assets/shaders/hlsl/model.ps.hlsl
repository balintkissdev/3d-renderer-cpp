#define ADS_FLAG_DIFFUSE_ENABLED  1
#define ADS_FLAG_SPECULAR_ENABLED 2

cbuffer MaterialBuffer : register(b1)
{
    float3 cbColor;
    uint cbAdsPropertiesFlags;
    float3 cbViewPos;
    float padding;
    float3 cbLightDirection;
};

struct PSInput
{
    float4 screenPosition : SV_POSITION;
    float3 worldPosition : POSITION;
    float3 normal : NORMAL;
};

float3 createDiffuse(float3 normal, float3 lightDirection)
{
    if ((cbAdsPropertiesFlags & ADS_FLAG_DIFFUSE_ENABLED) == 0)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }

    float diff = saturate(dot(normal, lightDirection));
    float3 diffuse = diff * cbColor;
    return diffuse;
}

float3 createSpecular(float3 normal, float3 lightDirection, float3 worldPosition)
{
    if ((cbAdsPropertiesFlags & ADS_FLAG_SPECULAR_ENABLED) == 0)
    {
        return float3(0.0f, 0.0f, 0.0f);
    }

    float3 viewDirercion = normalize(cbViewPos - worldPosition);
    float3 reflectDirection = reflect(-lightDirection, normal);
    float spec = pow(saturate(dot(viewDirercion, reflectDirection)), 64.0f);
    float3 specular = 1.0 * spec * cbColor;
    return specular;
}

float4 main(PSInput input) : SV_TARGET
{
    float ambientStrength = 0.2;
    float3 ambient = ambientStrength * cbColor;

    float3 lightDirection = normalize(-cbLightDirection);
    float3 diffuse =  createDiffuse(input.normal, lightDirection);
    float3 specular = createSpecular(input.normal, lightDirection, input.worldPosition);

    float3 result = ambient + diffuse + specular;

    return float4(result, 1.0f);
}
