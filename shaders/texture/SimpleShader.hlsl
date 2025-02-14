

struct Transform
{
    float4x4 ModelViewProjection;
    float DeltaTime;
    float Time;
};

ConstantBuffer<Transform> TransformData : register(b0);
SamplerState LinearSampler : register(s0);
Texture2D gTexture : register(t0);


struct VertexInput
{
    float3 Position : POSITION;
    float4 Color : COLOR;
    float2 TexCoord : TEXCOORD;
};

struct VertexOutput
{
    float4 Position : SV_Position;
    float4 Color : COLOR;
    float2 TexCoord : TEXCOORD;
};

VertexOutput VSMain(VertexInput input)
{
    float3 position = input.Position;
    position.y += sin(TransformData.Time);
    VertexOutput output;
    output.Position = mul(TransformData.ModelViewProjection, float4(position, 1.0f));
    output.Color = input.Color;
    output.TexCoord = input.TexCoord;

    return output;
}

float4 PSMain(VertexOutput input) : SV_Target
{
    float4 diffuse = gTexture.Sample(LinearSampler, input.TexCoord);
    //diffuse *= input.Color;

    return diffuse;
}
