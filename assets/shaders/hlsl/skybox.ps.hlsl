struct VSOutput
{
    float4 position : SV_POSITION;
    float3 texCoord : TEXCOORD0;
};

TextureCube  skyboxTexture : register(t0);
SamplerState skyboxSampler : register(s0);

float4 main(VSOutput input) : SV_TARGET0
{
    return skyboxTexture.Sample(skyboxSampler, input.texCoord);
}

