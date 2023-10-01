#include <Common.hlsli>

struct VSToGS
{
    float4 Pos : SV_POSITION;
    nointerpolation uint VertexId : VERTEXID;
};

struct GSToPS
{
    float4 Pos : SV_POSITION;
    float3 TexPos : TEXCOORD0; 
    nointerpolation uint Selection : TEXCOORD1;
    nointerpolation float4 Opacity : TEXCOORD2; 
};

VSToGS LensFlareVS(uint vertexId : SV_VERTEXID)
{
    VSToGS output = (VSToGS) 0;
    output.Pos = 0;
    output.VertexId = vertexId;
    return output;
}


Texture2D LensTx0 : register(t0);
Texture2D LensTx1 : register(t1);
Texture2D LensTx2 : register(t2);
Texture2D LensTx3 : register(t3);
Texture2D LensTx4 : register(t4);
Texture2D LensTx5 : register(t5);
Texture2D LensTx6 : register(t6);
Texture2D DepthTx : register(t7);

void AppendToStream(inout TriangleStream<GSToPS> triStream, GSToPS p1, uint selector, float2 posMod, float2 size)
{
    float2 pos = (lightData.screenSpacePosition.xy - 0.5) * float2(2, -2);
    float2 moddedPos = pos * posMod;
    float dis = distance(pos, moddedPos);

    p1.Pos.xy = moddedPos + float2(-size.x, -size.y);
    p1.TexPos.z = dis;
    p1.Selection = selector;
    p1.TexPos.xy = float2(0, 0);
    triStream.Append(p1);
	
    p1.Pos.xy = moddedPos + float2(-size.x, size.y);
    p1.TexPos.xy = float2(0, 1);
    triStream.Append(p1);

    p1.Pos.xy = moddedPos + float2(size.x, -size.y);
    p1.TexPos.xy = float2(1, 0);
    triStream.Append(p1);
	
    p1.Pos.xy = moddedPos + float2(size.x, size.y);
    p1.TexPos.xy = float2(1, 1);
    triStream.Append(p1);
}


[maxvertexcount(4)]
void LensFlareGS(
    point VSToGS p[1], inout TriangleStream<GSToPS> triStream)
{
    GSToPS output = (GSToPS) 0;
	float2 flareSize = float2(256, 256);
	[branch]
    switch (p[0].VertexId)
    {
        case 0:
            LensTx0.GetDimensions(flareSize.x, flareSize.y);
            break;
        case 1:
            LensTx1.GetDimensions(flareSize.x, flareSize.y);
            break;
        case 2:
            LensTx2.GetDimensions(flareSize.x, flareSize.y);
            break;
        case 3:
            LensTx3.GetDimensions(flareSize.x, flareSize.y);
            break;
        case 4:
            LensTx4.GetDimensions(flareSize.x, flareSize.y);
            break;
        case 5:
            LensTx5.GetDimensions(flareSize.x, flareSize.y);
            break;
        case 6:
            LensTx6.GetDimensions(flareSize.x, flareSize.y);
            break;
        default:
            break;
    };
    
    uint width, height, levels;
    DepthTx.GetDimensions(0, width, height, levels);
    
    float2 ScreenResolution = float2(width, height);
    flareSize /= ScreenResolution;
    float referenceDepth = saturate(lightData.screenSpacePosition.z);

	const float2 step = 1.0f / ScreenResolution;
    const float2 range = 10.5f * step;
    float samples = 0.0f;
    float depthAccumulated = 0.0f;
    for (float y = -range.y; y <= range.y; y += step.y)
    {
        for (float x = -range.x; x <= range.x; x += step.x)
        {
            samples += 1.0f;
            depthAccumulated += DepthTx.SampleLevel(PointClampSampler, lightData.screenSpacePosition.xy + float2(x, y), 0).r >= referenceDepth - 0.001 ? 1 : 0;
        }
    }
    depthAccumulated /= samples;
    output.Pos = float4(0, 0, 0, 1);
    output.Opacity = float4(depthAccumulated, 0, 0, 0);

	[branch]
    if (depthAccumulated > 0)
    {
      const float MODS[] = { 1, 0.55, 0.4, 0.1, -0.1, -0.3, -0.5 };
      AppendToStream(triStream, output, p[0].VertexId, MODS[p[0].VertexId], flareSize);
    }
}

float4 LensFlarePS(GSToPS input) : SV_TARGET
{
    float4 color = 0;
	
	[branch]
    switch (input.Selection)
    {
        case 0:
            color = float4(0.0f, 0.0f, 0.0f, 1.0f);
            break;
        case 1:
            color = LensTx1.SampleLevel(PointClampSampler, input.TexPos.xy, 0);
            break;
        case 2:
            color = LensTx2.SampleLevel(PointClampSampler, input.TexPos.xy, 0);
            break;
        case 3:
            color = LensTx3.SampleLevel(PointClampSampler, input.TexPos.xy, 0);
            break;
        case 4:
            color = LensTx4.SampleLevel(PointClampSampler, input.TexPos.xy, 0);
            break;
        case 5:
            color = LensTx5.SampleLevel(PointClampSampler, input.TexPos.xy, 0);
            break;
        case 6:
            color = LensTx6.SampleLevel(PointClampSampler, input.TexPos.xy, 0);
            break;
        default:
            break;
    };

    color *= 1.1 - saturate(input.TexPos.z);
    color *= input.Opacity.x;

    return color;
}