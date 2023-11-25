cbuffer SceneConstantBuffer : register(b0)
{
  matrix view;
  matrix projection;
};

struct Input
{
  float4 Position : SV_POSITION;
  float4 Color : COLOR;
};

Input VsMain(const float4 position : POSITION, const float4 color : COLOR)
{
  const matrix viewProj = mul(view, projection);

  Input result;
  result.Position = mul(position, viewProj);
  result.Color = color;

  return result;
}

float4 PsMain(const Input input, float3 baryWeights: SV_Barycentrics) : SV_TARGET
{
  if (baryWeights.x < 0.01f || baryWeights.y < 0.01f || baryWeights.z < 0.01f)
  {
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
  }
  else {
    return input.Color;
  }
}
