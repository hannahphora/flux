layout(set = 0, binding = 3) RWTexture2D<float4> images[];

static const uint TEXTURE_ID = 0;

[shader("compute")]
[numthreads(16, 16, 1)]
void main(
    uint3 dispatchID   : SV_DispatchThreadID,
    uint3 groupID      : SV_GroupThreadID
)
{
    int2 texelCoord = int2(dispatchID.xy);

    uint2 size;
    images[TEXTURE_ID].GetDimensions(size.x, size.y);

    if (texelCoord.x < size.x && texelCoord.y < size.y) {
        float4 color = float4(0, 0, 0, 1);

        // skip first row and col in each 16×16 tile
        if (groupID.x != 0 && groupID.y != 0)
        {
            // encode normalized UV in RG channels
            color.x = texelCoord.x / (float)size.x;
            color.y = texelCoord.y / (float)size.y;
        }

        images[TEXTURE_ID][texelCoord] = color;
    }
}