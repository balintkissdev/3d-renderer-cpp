#include "model.hlsli"
#include "mvp.hlsli"

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
};

VSOut main_vs(VSInput input)
{
    VSOut output;

    float4 worldPosition = mul(float4(input.position, 1.0f), cb_model);
    output.worldPosition = worldPosition.xyz;
    output.screenPosition = mul(mul(worldPosition, cb_view), cb_projection);

    output.normal = normalize(mul(input.normal, (float3x3)cb_normalMatrix));

    return output;
}
