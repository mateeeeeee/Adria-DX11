#include <Common.hlsli>

struct VSInput
{
    float3 Pos : POSITION;
    float2 Uvs : TEX;
};

struct VSToHS
{
    float4 WorldPos : POS;
    float2 TexCoord : TEX;
};

struct HSToDS
{
    float3 WorldPos : POS;
    float2 TexCoord : TEX;
};

struct DSToPS
{
	float4 Position	    : SV_POSITION;
    float4 WorldPos		: POS;
    float2 TexCoord		: TEX;
};

struct HSConstantDataOutput
{
	float EdgeTessFactor[3]			: SV_TessFactor;
	float InsideTessFactor			: SV_InsideTessFactor;
};

VSToHS OceanLodVS(VSInput vin)
{
    VSToHS output = (VSToHS)0;
    output.WorldPos = mul(float4(vin.Pos, 1.0), objectData.model);
    output.WorldPos /= output.WorldPos.w;
    output.TexCoord = vin.Uvs;
    return output;
}


#define NUM_CONTROL_POINTS 3

static const float MinDist = 100.0;
static const float MaxDist = 400.0;

static const float MinTess = 0;
static const float MaxTess = 4;

int CalcTessFactor(float3 p)
{
    float d = distance(p, frameData.cameraPosition.xyz);
    float s = saturate((d - MinDist) / (MaxDist - MinDist));
    return pow(2, lerp(MaxTess, MinTess, s));
}

HSConstantDataOutput CalcHSPatchConstants(
	InputPatch<VSToHS, NUM_CONTROL_POINTS> inputPatch,
	uint PatchID : SV_PrimitiveID)
{
	HSConstantDataOutput output = (HSConstantDataOutput)0;
    float3 e0 = 0.5 *   (inputPatch[1].WorldPos.xyz + inputPatch[2].WorldPos.xyz);
    float3 e1 = 0.5 *   (inputPatch[2].WorldPos.xyz + inputPatch[0].WorldPos.xyz);
    float3 e2 = 0.5 *   (inputPatch[0].WorldPos.xyz + inputPatch[1].WorldPos.xyz);
    float3 e3 = 1 / 3 * (inputPatch[0].WorldPos.xyz + inputPatch[1].WorldPos.xyz + inputPatch[2].WorldPos.xyz);
        
    output.EdgeTessFactor[0] = CalcTessFactor(e0);
    output.EdgeTessFactor[1] = CalcTessFactor(e1);
    output.EdgeTessFactor[2] = CalcTessFactor(e2);
    output.InsideTessFactor  = CalcTessFactor(e3);

	return output;
}

[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(NUM_CONTROL_POINTS)]
[patchconstantfunc("CalcHSPatchConstants")]
HSToDS OceanLodHS( 
	InputPatch<VSToHS, NUM_CONTROL_POINTS> inputPatch, 
	uint controlPointId : SV_OutputControlPointID,
	uint patchId : SV_PrimitiveID )
{
	HSToDS output = (HSToDS)0;
    output.WorldPos = inputPatch[controlPointId].WorldPos.xyz;
    output.TexCoord = inputPatch[controlPointId].TexCoord;
	return output;
}

static const float LAMBDA = 1.2f;
Texture2D DisplacementTx : register(t0);

[domain("tri")]
DSToPS OceanLodDS(
	HSConstantDataOutput input,
	float3 domain : SV_DomainLocation,
	const OutputPatch<HSToDS, NUM_CONTROL_POINTS> patch)
{
	DSToPS output = (DSToPS)0;
    output.TexCoord =
                        domain.x * patch[0].TexCoord +
                        domain.y * patch[1].TexCoord +
                        domain.z * patch[2].TexCoord;
    
    float3 WorldPos =   domain.x * patch[0].WorldPos +
                        domain.y * patch[1].WorldPos +
                        domain.z * patch[2].WorldPos;

    float3 dx = DisplacementTx.SampleLevel(LinearWrapSampler, output.TexCoord, 0.0f).xyz;
    WorldPos += dx * LAMBDA;
    output.WorldPos = float4(WorldPos, 1.0f);
    output.Position = mul(output.WorldPos, frameData.viewprojection);
	return output;
}