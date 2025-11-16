// TODO: Make view/projection separate CB
cbuffer MVPConstantBuffer : register(b0)
{
    float4x4 cb_model;
    float4x4 cb_view;
    float4x4 cb_projection;
    // Normal matrix is used for correctly transforming the input vertex normals to
    // world space. A normal matrix is the transpose of the
    // inverse of the upper-left 3x3 portion of the model matrix.
    //
    // normalMatrix = mat3(transpose(inverse(modelMatrix)))
    //
    float4x4 cb_normalMatrix;
};

