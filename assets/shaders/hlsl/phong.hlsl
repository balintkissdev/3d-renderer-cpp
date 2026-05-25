#include "classic_ads.hlsli"
#include "mvp.hlsli"

struct VSInput
{
    float3 vertexPosition : POSITION;
    float3 vertexNormal   : NORMAL;
};

struct VSOut
{
    float4 screenPosition   : SV_POSITION;
    float3 worldPosition    : POSITION;
    float3 worldNormal      : NORMAL;
};

VSOut main_vs(VSInput input)
{
    VSOut output;

    float4 worldPosition = mul(float4(input.vertexPosition, 1.0f), cb_model);
    output.worldPosition = worldPosition.xyz;
    output.screenPosition = mul(mul(worldPosition, cb_view), cb_projection);

    output.worldNormal = convertToWorldNormal(input.vertexNormal, cb_normalMatrix);

    return output;
}

float4 main_ps(VSOut input) : SV_TARGET0
{
    float3 rgb = classicADSLightModel(input.worldPosition, input.worldNormal);
    return float4(rgb, 1.0f);
}
