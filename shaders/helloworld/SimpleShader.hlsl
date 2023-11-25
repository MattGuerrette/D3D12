
cbuffer SceneConstantBuffer : register(b0)
{
    matrix modelViewProjection;
};

struct Input
{
    float4 Position : SV_POSITION;
    float4 Color : COLOR;
};

Input VSMain(float4 position : POSITION, float4 color : COLOR)
{
    Input result;
    result.Position = mul(modelViewProjection, position);
    result.Color = color;

    return result;
}

float4 PSMain(const Input input, float3 baryWeights : SV_Barycentrics) : SV_TARGET
{
    if (baryWeights.x < 0.01f || baryWeights.y < 0.01f || baryWeights.z < 0.01f)
    {
        return float4(1.0f, 1.0f, 1.0f, 1.0f);
    }
    else
    {
        return input.Color;
    }
}
