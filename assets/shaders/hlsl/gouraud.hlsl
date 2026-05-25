#include "mvp.hlsli"
#include "classic_ads.hlsli"

struct VSInput
{
    float3 vertexPosition : POSITION;
    float3 vertexNormal   : NORMAL;
};

struct VSOut
{
    float4 screenPosition : SV_POSITION;
    float3 rgb            : COLOR;
};

VSOut main_vs(VSInput input)
{
    VSOut output;

    float4 worldPosition = mul(float4(input.vertexPosition, 1.0f), cb_model);
    output.screenPosition = mul(mul(worldPosition, cb_view), cb_projection);

    float3 worldNormal = convertToWorldNormal(input.vertexNormal, cb_normalMatrix);

    output.rgb = classicADSLightModel(worldPosition, worldNormal);

    return output;
}

float4 main_ps(VSOut output) : SV_TARGET0
{
    return float4(output.rgb, 1.0f);
}
