// TODO: Make view/projection separate CB
cbuffer MVPConstantBuffer : register(b0)
{
    matrix model;
    matrix view;
    matrix projection;
    matrix normalMatrix;
};

struct VSOutput
{
    float4 position : SV_POSITION;
    float3 texCoord : TEXCOORD0;
};

VSOutput main(float3 position : POSITION)
{
    matrix viewRotation = view;
    viewRotation[3] = float4(0, 0, 0, 1);

    float4 pos = mul(float4(position, 1.0), viewRotation);
    pos = mul(pos, projection);

    VSOutput output;
    output.position = pos.xyww;
    output.texCoord = position;
    return output;
}

