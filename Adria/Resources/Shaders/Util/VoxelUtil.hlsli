#ifndef _VOXEL_UTIL_
#define _VOXEL_UTIL_


//https://github.com/turanszkij/WickedEngine

struct VoxelRadiance
{
    float3  GridCenter;
    float   DataSize;       // voxel half-extent in world space units
    float   DataSizeRCP;    // 1.0 / voxel-half extent
    uint    DataRes;        // voxel grid resolution
    float   DataResRCP;     // 1.0 / voxel grid resolution
    uint    NumCones;
    float   NumConesRCP;
    float   MaxDistance;
    float   RayStepSize;
    uint    Mips;
};

struct VoxelType
{
    uint ColorMask;
    uint NormalMask;
};

static const float HDR_RANGE = 10.0f;
static const float MAX_VOXEL_LIGHTS = 8;

uint EncodeColor(float4 color)
{
	float hdr = length(color.rgb);
    color.rgb /= hdr;

	uint3 iColor = uint3(color.rgb * 255.0f);
    uint iHDR = (uint) (saturate(hdr / HDR_RANGE) * 127);
    uint colorMask = (iHDR << 24u) | (iColor.r << 16u) | (iColor.g << 8u) | iColor.b;

	uint iAlpha = (color.a > 0 ? 1u : 0u);
    colorMask |= iAlpha << 31u;
    return colorMask;
}

float4 DecodeColor(uint colorMask)
{
    float hdr;
    float4 color;

    hdr = (colorMask >> 24u) & 0x0000007f;
    color.r = (colorMask >> 16u) & 0x000000ff;
    color.g = (colorMask >> 8u) & 0x000000ff;
    color.b = colorMask & 0x000000ff;

    hdr /= 127.0f;

    color.rgb /= 255.0f;
    color.rgb *= hdr * HDR_RANGE;
    color.a = (colorMask >> 31u) & 0x00000001;

    return color;
}

uint EncodeNormal(float3 normal)
{
    int3 iNormal = int3(normal * 255.0f);
    uint3 iNormalSigns;
    iNormalSigns.x = (iNormal.x >> 5) & 0x04000000;
    iNormalSigns.y = (iNormal.y >> 14) & 0x00020000;
    iNormalSigns.z = (iNormal.z >> 23) & 0x00000100;
    iNormal = abs(iNormal);
    uint normalMask = iNormalSigns.x | (iNormal.x << 18) | iNormalSigns.y | (iNormal.y << 9) | iNormalSigns.z | iNormal.z;
    return normalMask;
}


float3 DecodeNormal(uint normalMask)
{
    int3 iNormal;
    iNormal.x = (normalMask >> 18) & 0x000000ff;
    iNormal.y = (normalMask >> 9) & 0x000000ff;
    iNormal.z = normalMask & 0x000000ff;
    int3 iNormalSigns;
    iNormalSigns.x = (normalMask >> 25) & 0x00000002;
    iNormalSigns.y = (normalMask >> 16) & 0x00000002;
    iNormalSigns.z = (normalMask >> 7) & 0x00000002;
    iNormalSigns = 1 - iNormalSigns;
    float3 normal = float3(iNormal) / 255.0f;
    normal *= iNormalSigns;
    return normal;
}

uint Flatten3D(uint3 coord, uint3 dim)
{
    return (coord.z * dim.x * dim.y) + (coord.y * dim.x) + coord.x;
}
uint3 Unflatten3D(uint idx, uint3 dim)
{
    const uint z = idx / (dim.x * dim.y);
    idx -= (z * dim.x * dim.y);
    const uint y = idx / dim.x;
    const uint x = idx % dim.x;
    return uint3(x, y, z);
}

#endif