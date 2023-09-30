

float CalcShadowFactor_Basic(SamplerComparisonState shadowSampler, Texture2D shadowMap, float3 uvd)
{
    if (uvd.z > 1.0f) return 1.0;
    return shadowMap.SampleCmpLevelZero(shadowSampler,
		uvd.xy, uvd.z).r;
}


float CalcShadowFactor_PCF3x3(SamplerComparisonState shadowSampler, Texture2D shadowMap, 
							  float3 uvd, int smSize, float softness)
{
    if (uvd.z > 1.0f) return 1.0;

    float depth = uvd.z;
    const float dx = 1.0f / smSize;
    float percentLit = 0.0f;

    float2 offsets[9] =
    {
        float2(-dx, -dx), float2(0.0f, -dx), float2(dx, -dx),
		float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
		float2(-dx, +dx), float2(0.0f, +dx), float2(dx, +dx)
    };

	[unroll]
    for (int i = 0; i < 9; ++i)
    {
        offsets[i] = offsets[i] * float2(softness, softness);
        percentLit += shadowMap.SampleCmpLevelZero(shadowSampler,
			uvd.xy + offsets[i], depth).r;
    }
    return percentLit /= 9.0f;
}

float CSMCalcShadowFactor_Basic(SamplerComparisonState shadowSampler, Texture2DArray shadowMap, 
								uint index, float3 uvd, int smSize, float softness)
{
    return shadowMap.SampleCmpLevelZero(shadowSampler,
		float3(uvd.xy, index), uvd.z).r;
}

float CSMCalcShadowFactor_PCF3x3(SamplerComparisonState shadowSampler, Texture2DArray shadowMap, 
								 uint arrayIndex, float3 uvd, int smSize, float softness)
{
    const float dx = 1.0f / smSize;
    float percentLit = 0.0f;

    const float2 offsets[9] =
    {
        float2(-dx, -dx), float2(0.0f, -dx), float2(dx, -dx),
		float2(-dx, 0.0f), float2(0.0f, 0.0f), float2(dx, 0.0f),
		float2(-dx, +dx), float2(0.0f, +dx), float2(dx, +dx)
    };

	[unroll]
    for (int i = 0; i < 9; ++i)
    {
        percentLit += shadowMap.SampleCmpLevelZero(shadowSampler,
			float3(uvd.xy + offsets[i], arrayIndex), uvd.z).r;
    }
    return percentLit /= 9.0f;
}


float CalcShadowFactor_PCF5x5(SamplerComparisonState shadowSampler, Texture2D shadowMap,
							  float3 shadowPosH, int smSize, float softness)
{
    if (shadowPosH.z > 1.0f) return 1.0;
	
    float depth = shadowPosH.z;
    const float dx = 1.0f / smSize;

    float percentLit = 0.0f;
    float2 offsets[25] =
    {
        float2(-2 * dx, -2 * dx), float2(-1 * dx, -2 * dx), float2(0 * dx, -2 * dx), float2(1 * dx, -2 * dx), float2(2 * dx, -2 * dx),
		float2(-2 * dx, -1 * dx), float2(-1 * dx, -1 * dx), float2(0 * dx, -1 * dx), float2(1 * dx, -1 * dx), float2(2 * dx, -1 * dx),
		float2(-2 * dx, 0 * dx), float2(-1 * dx, 0 * dx), float2(0 * dx, 0 * dx), float2(1 * dx, 0 * dx), float2(2 * dx, 0 * dx),
		float2(-2 * dx, 1 * dx), float2(-1 * dx, 1 * dx), float2(0 * dx, 1 * dx), float2(1 * dx, 1 * dx), float2(2 * dx, 1 * dx),
		float2(-2 * dx, 2 * dx), float2(-1 * dx, 2 * dx), float2(0 * dx, 2 * dx), float2(1 * dx, 2 * dx), float2(2 * dx, 2 * dx),
    };

	[unroll]
    for (int i = 0; i < 25; ++i)
    {
        offsets[i] = offsets[i] * float2(softness, softness);
        percentLit += shadowMap.SampleCmpLevelZero(shadowSampler,
			shadowPosH.xy + offsets[i], depth).r;
    }
    return percentLit /= 25.0f;
}

float CalcShadowFactor_PCF7x7(SamplerComparisonState shadowSampler, Texture2D shadowMap,
							  float3 shadowPosH, int smSize, float softness)
{
    if (shadowPosH.z > 1.0f) return 1.0;
   
    float depth = shadowPosH.z;
    const float dx = 1.0f / smSize;
    float percentLit = 0.0f;

	[unroll]
    for (int i = -3; i <= 3; ++i)
    {
        for (int j = -3; j <= 3; ++j)
        {
            percentLit += shadowMap.SampleCmpLevelZero(shadowSampler,
				shadowPosH.xy + float2(i * dx * softness, j * dx * softness), depth).r;
        }
    }
    return percentLit /= 49;
}

static const float2 PoissonSamples[64] =
{
    float2(-0.5119625f, -0.4827938f),
	float2(-0.2171264f, -0.4768726f),
	float2(-0.7552931f, -0.2426507f),
	float2(-0.7136765f, -0.4496614f),
	float2(-0.5938849f, -0.6895654f),
	float2(-0.3148003f, -0.7047654f),
	float2(-0.42215f, -0.2024607f),
	float2(-0.9466816f, -0.2014508f),
	float2(-0.8409063f, -0.03465778f),
	float2(-0.6517572f, -0.07476326f),
	float2(-0.1041822f, -0.02521214f),
	float2(-0.3042712f, -0.02195431f),
	float2(-0.5082307f, 0.1079806f),
	float2(-0.08429877f, -0.2316298f),
	float2(-0.9879128f, 0.1113683f),
	float2(-0.3859636f, 0.3363545f),
	float2(-0.1925334f, 0.1787288f),
	float2(0.003256182f, 0.138135f),
	float2(-0.8706837f, 0.3010679f),
	float2(-0.6982038f, 0.1904326f),
	float2(0.1975043f, 0.2221317f),
	float2(0.1507788f, 0.4204168f),
	float2(0.3514056f, 0.09865579f),
	float2(0.1558783f, -0.08460935f),
	float2(-0.0684978f, 0.4461993f),
	float2(0.3780522f, 0.3478679f),
	float2(0.3956799f, -0.1469177f),
	float2(0.5838975f, 0.1054943f),
	float2(0.6155105f, 0.3245716f),
	float2(0.3928624f, -0.4417621f),
	float2(0.1749884f, -0.4202175f),
	float2(0.6813727f, -0.2424808f),
	float2(-0.6707711f, 0.4912741f),
	float2(0.0005130528f, -0.8058334f),
	float2(0.02703013f, -0.6010728f),
	float2(-0.1658188f, -0.9695674f),
	float2(0.4060591f, -0.7100726f),
	float2(0.7713396f, -0.4713659f),
	float2(0.573212f, -0.51544f),
	float2(-0.3448896f, -0.9046497f),
	float2(0.1268544f, -0.9874692f),
	float2(0.7418533f, -0.6667366f),
	float2(0.3492522f, 0.5924662f),
	float2(0.5679897f, 0.5343465f),
	float2(0.5663417f, 0.7708698f),
	float2(0.7375497f, 0.6691415f),
	float2(0.2271994f, -0.6163502f),
	float2(0.2312844f, 0.8725659f),
	float2(0.4216993f, 0.9002838f),
	float2(0.4262091f, -0.9013284f),
	float2(0.2001408f, -0.808381f),
	float2(0.149394f, 0.6650763f),
	float2(-0.09640376f, 0.9843736f),
	float2(0.7682328f, -0.07273844f),
	float2(0.04146584f, 0.8313184f),
	float2(0.9705266f, -0.1143304f),
	float2(0.9670017f, 0.1293385f),
	float2(0.9015037f, -0.3306949f),
	float2(-0.5085648f, 0.7534177f),
	float2(0.9055501f, 0.3758393f),
	float2(0.7599946f, 0.1809109f),
	float2(-0.2483695f, 0.7942952f),
	float2(-0.4241052f, 0.5581087f),
	float2(-0.1020106f, 0.6724468f),
};

float CalcShadowFactor_Poisson(SamplerComparisonState shadowSampler, Texture2D shadowMap, 
							   float4 shadowPosH, int smSize, float softness)
{
    if (shadowPosH.z > 1.0f) return 1.0;

    float depth = shadowPosH.z;
    const float dx = 1.0f / smSize;
    float percentLit = 0.0f;

	[unroll]
    for (int i = 0; i < 64; ++i)
    {
        percentLit += shadowMap.SampleCmpLevelZero(shadowSampler,
			shadowPosH.xy + PoissonSamples[i] * float2(dx * softness, dx * softness), depth).r;
    }
    return percentLit /= 64;
}
