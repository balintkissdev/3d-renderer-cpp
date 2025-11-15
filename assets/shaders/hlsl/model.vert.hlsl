cbuffer MVPConstantBuffer : register(b0)
{
    matrix cb_model;
    matrix cb_view;
    matrix cb_projection;
    // Normal matrix is used for correctly transform the input vertex normals to
    // world space. A normal matrix is the transpose of the
    // inverse of the upper-left 3x3 portion of the model matrix.
    //
    // normalMatrix = mat3(transpose(inverse(modelMatrix)))
    //
    matrix cb_normalMatrix;
};

struct VSInput
{
    float3 position : POSITION;
    float3 normal : NORMAL;
};

struct VSOut
{
    float4 screenPosition : SV_POSITION;
    float3 worldPosition : POSITION;
    float3 normal : NORMAL;
};

VSOut main(VSInput input)
{
    VSOut result;

    float4 worldPosition = mul(float4(input.position, 1.0f), cb_model);
    result.worldPosition = worldPosition.xyz;
    result.screenPosition = mul(mul(worldPosition, cb_view), cb_projection);

    result.normal = normalize(mul(input.normal, (float3x3)cb_normalMatrix));

    return result;
}
