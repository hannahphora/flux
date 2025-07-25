RWTexture2D<float4> image : register(u0, space0);

[shader("compute"), numthreads(16, 16, 1)]
void computeMain(
    uint3 dispatchThreadID: SV_DispatchThreadID, // absolute thread index
    uint3 groupThreadID: SV_GroupThreadID        // local thread index
)
{
    uint2 texelCoord = dispatchThreadID.xy; // 2d texel coord from dispatch ID

    uint width, height;
    image.GetDimensions(width, height);

    if (texelCoord.x < width && texelCoord.y < height) // write if inside bounds
    {
        float4 color = float4(0, 0, 0, 1);
        if (groupThreadID.x != 0 && groupThreadID.y != 0) // skip first row/column
        {
            color.x = texelCoord.x / float(width);
            color.y = texelCoord.y / float(height);
        }
        image[texelCoord] = color;
    }
}