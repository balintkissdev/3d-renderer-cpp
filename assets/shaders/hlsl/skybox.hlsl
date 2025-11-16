#include "mvp.hlsli"

struct VSOut
{
    float4 position : SV_POSITION;
    float3 uv : TEXCOORD;
};

VSOut main_vs(float3 position : POSITION)
{
    float4x4 viewRotation = cb_view;
    viewRotation[3] = float4(0, 0, 0, 1);

    float4 pos = mul(float4(position, 1.0), viewRotation);
    pos = mul(pos, cb_projection);

    VSOut output;
    output.position = pos.xyww;
    output.uv = position;
    return output;
}

TextureCube  skyboxTexture : register(t0);
SamplerState skyboxSampler : register(s0);

float4 main_ps(VSOut input) : SV_TARGET0
{
    return skyboxTexture.Sample(skyboxSampler, input.uv);
}

