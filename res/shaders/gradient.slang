layout(set = 0, binding = 2) RWTexture2D<float4> images[];

static const uint TEXTURE_ID = 0;

RWTexture2D<float4> result = images[TEXTURE_ID];

[shader("compute")]
[numthreads(16, 16, 1)]
void main(
    uint3 dispatchID   : SV_DispatchThreadID,   // gl_GlobalInvocationID
    uint3 groupID      : SV_GroupThreadID       // gl_LocalInvocationID
)
{
    // Compute our texel coordinate
    int2 texelCoord = int2(dispatchID.xy);

    uint2 size;
    images[TEXTURE_ID].GetDimensions(size.x, size.y);

    // Bounds check
    if (texelCoord.x < size.x && texelCoord.y < size.y)
    {
        // Default to opaque black
        float4 color = float4(0, 0, 0, 1);

        // Skip the first row and column within each 16×16 tile
        if (groupID.x != 0 && groupID.y != 0)
        {
            // Encode normalized UV into RG channels
            color.x = texelCoord.x / (float)size.x;
            color.y = texelCoord.y / (float)size.y;
        }

        result[texelCoord] = color;
    }
}