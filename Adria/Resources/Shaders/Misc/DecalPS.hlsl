#include "../Globals/GlobalsPS.hlsli"

Texture2D<float4> txAlbedoDecal : register(t0);
Texture2D<float4> txNormalDecal : register(t1);
Texture2D<float>  txDepth       : register(t2);

struct PS_DECAL_OUT
{
    float4 DiffuseRoughness : SV_TARGET0;
};

struct PS_INPUT
{
    float4 Position     : SV_POSITION;
    float4 ClipSpacePos : POSITION;
    matrix InverseModel : INVERSE_MODEL;
};

cbuffer DecalCBuffer : register(b11)
{
    row_major matrix decal_projection;
    row_major matrix decal_viewprojection;
    row_major matrix decal_inverse_viewprojection;
    float2 aspect_ratio;
}

PS_DECAL_OUT main(PS_INPUT input)
{
    PS_DECAL_OUT pout = (PS_DECAL_OUT) 0;
    //pout.DiffuseRoughness.rgb = float4(1.0f, 0.0f, 0.0f, 1.0f);
    //return pout;

    float2 screen_pos = input.ClipSpacePos.xy / input.ClipSpacePos.w;
    float2 depth_coords = screen_pos * float2(0.5f, -0.5f) + 0.5f;
    float depth = txDepth.Sample(point_clamp_sampler, depth_coords).r;

    float4 posVS = float4(GetPositionVS(depth_coords, depth), 1.0f);
    float4 posWS = mul(posVS, inverse_view);
    float4 posLS = mul(posWS, input.InverseModel);
    posLS.xyz /= posLS.w;

    clip(0.5f - abs(posLS.xyz));

    float2 tex_coords = posLS.xy + 0.5f;
    float4 albedo = txAlbedoDecal.Sample(linear_wrap_sampler, tex_coords);
    if (albedo.a < 0.1) discard;
    pout.DiffuseRoughness.rgb = albedo.rgb;
    return pout;
}

/*
vertex VertexOut vertex_decal(
    const VertexIn in [[ stage_in ]],
    constant DecalVertexUniforms &uniforms [[ buffer(2) ]]
) {
    VertexOut out;

    out.position = uniforms.projectionMatrix * uniforms.viewMatrix * uniforms.modelMatrix * in.position;
    out.viewPosition = (uniforms.viewMatrix * uniforms.modelMatrix * in.position).xyz;
    out.normal = uniforms.normalMatrix * in.normal;
    out.uv = in.uv;
    
    return out;
}
*/

/*
fragment float4 fragment_decal(
    const VertexOut in [[ stage_in ]],
    constant DecalFragmentUniforms &uniforms [[ buffer(3) ]],
    depth2d<float, access::sample> depthTexture [[ texture(0) ]],
    texture2d<float, access::sample> colorTexture [[ texture(1) ]]
) {
    constexpr sampler depthSampler (mag_filter::linear, min_filter::linear);

    float2 resolution = float2(
        depthTexture.get_width(),
        depthTexture.get_height()
    );
        
    float2 depthCoordinate = in.position.xy / resolution;
    float depth = depthTexture.sample(depthSampler, depthCoordinate);
    
    float3 screenPosition = float3((depthCoordinate.x * 2 - 1), -(depthCoordinate.y * 2 - 1), depth);
    float4 viewPosition = uniforms.inverseProjectionMatrix * float4(screenPosition, 1);
    float4 worldPosition = uniforms.inverseViewMatrix * viewPosition;
    float4 objectPosition = uniforms.inverseModelMatrix * worldPosition;
    float3 localPosition = objectPosition.xyz / objectPosition.w;
    
    if(abs(localPosition.x) > 0.5 || abs(localPosition.y) > 0.5 || abs(localPosition.z) > 0.5) {
        discard_fragment();
    } else {
        float2 textureCoordinate = localPosition.xy + 0.5;
        float4 color = colorTexture.sample(depthSampler, textureCoordinate);
        
        return float4(color.rgb, 1);
    }
}
}
*/