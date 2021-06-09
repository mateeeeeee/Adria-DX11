#include "../Globals/GlobalsPS.hlsli"

//https://gist.github.com/Erkaman/f24ef6bd7499be363e6c99d116d8734d

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 Tex : TEX;
};

/*
In my engine, this is the HDR image that we get AFTER the deferred lighting has been done,
and BEFORE the tonemapping is applied.
hdrTex = HDR image of current frame
histHdrTex = HDR image of previous frame.
*/
Texture2D hdr_scene : register(t0);
Texture2D prev_hdr_scene : register(t1);
//Texture2D velocity_buffer : register(t2);  for dynamic scenes, see MotionBlurPS on how to generate it

bool2 GreaterThan(float2 a, float2 b)
{
    return bool2(a.x > b.x, a.y > b.y);
}

bool2 LessThan(float2 a, float2 b)
{
    return bool2(a.x < b.x, a.y < b.y);
}

float4 main(VertexOut pin) : SV_TARGET
{
	float3 neighbourhood[9];
    
    uint width, height, dummy;
    
    hdr_scene.GetDimensions(0, width, height, dummy);
    
    float2 pixel_size = float2(1.0 / width, 1.0 / height);
        
    neighbourhood[0] = hdr_scene.Sample(linear_wrap_sampler, pin.Tex + float2(-1,-1) * pixel_size).rgb;//texture2D(hdrTex, fsUv.xy + vec2(-1, -1) * uPixelSize ).xyz;
    neighbourhood[1] = hdr_scene.Sample(linear_wrap_sampler, pin.Tex + float2(+0,-1) * pixel_size).rgb;//texture2D(hdrTex, fsUv.xy + vec2(+0, -1) * uPixelSize ).xyz;
    neighbourhood[2] = hdr_scene.Sample(linear_wrap_sampler, pin.Tex + float2(+1,-1) * pixel_size).rgb;//texture2D(hdrTex, fsUv.xy + vec2(+1, -1) * uPixelSize ).xyz;
    neighbourhood[3] = hdr_scene.Sample(linear_wrap_sampler, pin.Tex + float2(-1,+0) * pixel_size).rgb;//texture2D(hdrTex, fsUv.xy + vec2(-1, +0) * uPixelSize ).xyz;
    neighbourhood[4] = hdr_scene.Sample(linear_wrap_sampler, pin.Tex + float2(+0,+0) * pixel_size).rgb;//texture2D(hdrTex, fsUv.xy + vec2(+0, +0) * uPixelSize ).xyz;
    neighbourhood[5] = hdr_scene.Sample(linear_wrap_sampler, pin.Tex + float2(+1,+0) * pixel_size).rgb;//texture2D(hdrTex, fsUv.xy + vec2(+1, +0) * uPixelSize ).xyz;
    neighbourhood[6] = hdr_scene.Sample(linear_wrap_sampler, pin.Tex + float2(-1,+1) * pixel_size).rgb;//texture2D(hdrTex, fsUv.xy + vec2(-1, +1) * uPixelSize ).xyz;
    neighbourhood[7] = hdr_scene.Sample(linear_wrap_sampler, pin.Tex + float2(+0,+1) * pixel_size).rgb;//texture2D(hdrTex, fsUv.xy + vec2(+0, +1) * uPixelSize ).xyz;
    neighbourhood[8] = hdr_scene.Sample(linear_wrap_sampler, pin.Tex + float2(+1,+1) * pixel_size).rgb;//texture2D(hdrTex, fsUv.xy + vec2(+1, +1) * uPixelSize).xyz;

    float3 nmin = neighbourhood[0];
    float3 nmax = neighbourhood[0];
    for (int i = 1; i < 9; ++i)
    {
        nmin = min(nmin, neighbourhood[i]);
        nmax = max(nmax, neighbourhood[i]);
    }
    
    //float2 vel = texture2D(velocityTex, fsUv.xy).xy;
    float2 hist_uv = pin.Tex;// + vel.xy;
        
    float3 hist_sample = clamp(prev_hdr_scene.Sample(linear_wrap_sampler, pin.Tex).xyz, nmin, nmax);
    float blend = 0.05;
        
    bool2 a = GreaterThan(hist_uv, float2(1.0, 1.0));
    bool2 b = LessThan(hist_uv, float2(0.0, 0.0));
    // if history sample is outside screen, switch to aliased image as a fallback.
    blend = any(bool2(any(a), any(b))) ? 1.0 : blend;
        
    float3 cur_sample = neighbourhood[4];
    float3 c = lerp(hist_sample, cur_sample, blend);
    
    return float4(c, 1.0f);
}

/* https://github.com/NVIDIAGameWorks/Falcor/blob/master/Source/RenderPasses/Antialiasing/TAA/TAA.ps.slang
cbuffer PerFrameCB : register(b0)
{
    float gAlpha;
    float gColorBoxSigma;
};

Texture2D gTexColor;
Texture2D gTexMotionVec;
Texture2D gTexPrevColor;
SamplerState gSampler;


// Catmull-Rom filtering code from http://vec3.ca/bicubic-filtering-in-fewer-taps/
float3 bicubicSampleCatmullRom(Texture2D tex, SamplerState samp, float2 samplePos, float2 texDim)
{
    float2 invTextureSize = 1.0 / texDim;
    float2 tc = floor(samplePos - 0.5) + 0.5;
    float2 f = samplePos - tc;
    float2 f2 = f * f;
    float2 f3 = f2 * f;

    float2 w0 = f2 - 0.5 * (f3 + f);
    float2 w1 = 1.5 * f3 - 2.5 * f2 + 1;
    float2 w3 = 0.5 * (f3 - f2);
    float2 w2 = 1 - w0 - w1 - w3;

    float2 w12 = w1 + w2;
    
    float2 tc0 = (tc - 1) * invTextureSize;
    float2 tc12 = (tc + w2 / w12) * invTextureSize;
    float2 tc3 = (tc + 2) * invTextureSize;

    float3 result =
        tex.SampleLevel(samp, float2(tc0.x,  tc0.y), 0).rgb  * (w0.x  * w0.y) +
        tex.SampleLevel(samp, float2(tc0.x,  tc12.y), 0).rgb * (w0.x  * w12.y) +
        tex.SampleLevel(samp, float2(tc0.x,  tc3.y), 0).rgb  * (w0.x  * w3.y) +
        tex.SampleLevel(samp, float2(tc12.x, tc0.y), 0).rgb  * (w12.x * w0.y) +
        tex.SampleLevel(samp, float2(tc12.x, tc12.y), 0).rgb * (w12.x * w12.y) +
        tex.SampleLevel(samp, float2(tc12.x, tc3.y), 0).rgb  * (w12.x * w3.y) +
        tex.SampleLevel(samp, float2(tc3.x,  tc0.y), 0).rgb  * (w3.x  * w0.y) +
        tex.SampleLevel(samp, float2(tc3.x,  tc12.y), 0).rgb * (w3.x  * w12.y) +
        tex.SampleLevel(samp, float2(tc3.x,  tc3.y), 0).rgb  * (w3.x  * w3.y);

    return result;
}


float4 main(float2 texC : TEXCOORD) : SV_TARGET0
{
    const int2 offset[8] = { int2(-1, -1), int2(-1,  1),
                              int2( 1, -1), int2( 1,  1), 
                              int2( 1,  0), int2( 0, -1), 
                              int2( 0,  1), int2(-1,  0), };
    
    uint2 texDim;
    uint levels;
    gTexColor.GetDimensions(0, texDim.x, texDim.y, levels);

    float2 pos = texC * texDim;
    int2 ipos = int2(pos);

    // Fetch the current pixel color and compute the color bounding box
    // Details here: http://www.gdcvault.com/play/1023521/From-the-Lab-Bench-Real
    // and here: http://cwyman.org/papers/siga16_gazeTrackedFoveatedRendering.pdf
    float3 color = gTexColor.Load(int3(ipos, 0)).rgb;
    color = RGBToYCgCo(color);
    float3 colorAvg = color;
    float3 colorVar = color * color;
    [unroll]
    for (int k = 0; k < 8; k++) 
    {
        float3 c = gTexColor.Load(int3(ipos + offset[k], 0)).rgb;
        c = RGBToYCgCo(c);
        colorAvg += c;
        colorVar += c * c;
    }

    float oneOverNine = 1.0 / 9.0;
    colorAvg *= oneOverNine;
    colorVar *= oneOverNine;

    float3 sigma = sqrt(max(0.0f, colorVar - colorAvg * colorAvg));
    float3 colorMin = colorAvg - gColorBoxSigma * sigma;
    float3 colorMax = colorAvg + gColorBoxSigma * sigma;    

    // Find the longest motion vector
    float2 motion = gTexMotionVec.Load(int3(ipos, 0)).xy;
    [unroll]
    for (int a = 0; a < 8; a++) 
    {
        float2 m = gTexMotionVec.Load(int3(ipos + offset[a], 0)).rg;
        motion = dot(m, m) > dot(motion, motion) ? m : motion;   
    }

    // Use motion vector to fetch previous frame color (history)
    float3 history = bicubicSampleCatmullRom(gTexPrevColor, gSampler, (texC + motion) * texDim, texDim);

    history = RGBToYCgCo(history);

    // Anti-flickering, based on Brian Karis talk @Siggraph 2014
    // https://de45xmedrsdbp.cloudfront.net/Resources/files/TemporalAA_small-59732822.pdf
    // Reduce blend factor when history is near clamping
    float distToClamp = min(abs(colorMin.x - history.x), abs(colorMax.x - history.x));
    float alpha = clamp((gAlpha * distToClamp) / (distToClamp + colorMax.x - colorMin.x), 0.0f, 1.0f);

    history = clamp(history, colorMin, colorMax);
    float3 result = YCgCoToRGB(lerp(history, color, alpha));
    return float4(result, 0);
}
*/