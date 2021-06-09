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
        
    float3 hist_sample = clamp(hdr_scene.Sample(linear_wrap_sampler, pin.Tex).xyz, nmin, nmax);
    float blend = 0.05;
        
    bool2 a = GreaterThan(hist_uv, float2(1.0, 1.0));
    bool2 b = LessThan(hist_uv, float2(0.0, 0.0));
    // if history sample is outside screen, switch to aliased image as a fallback.
    blend = any(bool2(any(a), any(b))) ? 1.0 : blend;
        
    float3 curSample = neighbourhood[4];
    float3 c = lerp(hist_sample, curSample, blend);
    
    return float4(c, 1.0f);
}