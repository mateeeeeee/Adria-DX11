#include <common.hlsli>

struct VSToGS
{
    float2 Position : POSITION;
    float2 Size     : SIZE;
    float3 Color    : COLOR;
    float  Depth    : DEPTH;
};

struct GSToPS
{
    float4 PositionCS   : SV_Position;
    float2 TexCoord     : TEXCOORD;
    float3 Color : COLOR;
    float  Depth : DEPTH;
};


struct Bokeh
{
    float3 position;
    float2 size;
    float3 color;
};
StructuredBuffer<Bokeh> BokehStack : register(t0);

VSToGS BokehVS(in uint VertexID : SV_VertexID)
{
    VSToGS output = (VSToGS)0;
    Bokeh bokeh = BokehStack[VertexID];

	output.Position.xy = bokeh.position.xy;
    output.Position.xy = output.Position.xy * 2.0f - 1.0f;
    output.Position.y *= -1.0f;

	output.Size = bokeh.size; 
    output.Depth = bokeh.position.z; 
    output.Color = bokeh.color;
    return output;
}


static const float2 Offsets[4] =
{
    float2(-1, 1),
	float2(1, 1),
	float2(-1, -1),
	float2(1, -1)
};

static const float2 TexCoords[4] =
{
    float2(0, 0),
	float2(1, 0),
	float2(0, 1),
	float2(1, 1)
};

[maxvertexcount(4)]
void BokehGS(point VSToGS input[1], inout TriangleStream<GSToPS> SpriteStream)
{
    GSToPS output = (GSToPS)0;

	[unroll]
    for (int i = 0; i < 4; i++)
    {
        output.PositionCS = float4(input[0].Position.xy, 1.0f, 1.0f);
        output.PositionCS.xy += Offsets[i] * input[0].Size;
        output.TexCoord = TexCoords[i];
        output.Color = input[0].Color;
        output.Depth = input[0].Depth;

        SpriteStream.Append(output);
    }
    SpriteStream.RestartStrip();
}

Texture2D BokehTexture         : register(t0);

float4 BokehPS(GSToPS input) : SV_TARGET
{
    float bokehFactor = BokehTexture.Sample(LinearWrapSampler, input.TexCoord).r;
    return float4(input.Color * bokehFactor, 1.0f);
}