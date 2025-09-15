struct VSOut
{
    float4 position : SV_POSITION;
    float3 outColor : TEXCOORD0;
};

[shader("vertex")]
VSOut main(uint vertexID : SV_VertexID)
{
    VSOut out;

    static const float3 positions[3] = {
        float3( 1.0f,  1.0f, 0.0f),
        float3(-1.0f,  1.0f, 0.0f),
        float3( 0.0f, -1.0f, 0.0f)
    };

    static const float3 colors[3] = {
        float3(1.0f, 0.0f, 0.0f), // red
        float3(0.0f, 1.0f, 0.0f), // green
        float3(0.0f, 0.0f, 1.0f)  // blue
    };

    out.position = float4(positions[vertexID], 1.0f);
    out.outColor = colors[vertexID];

    return out;
}