
[shader("fragment")]
float4 main(float3 inColor : TEXCOORD0) : SV_Target
{
    return float4(inColor, 1.0f);
}