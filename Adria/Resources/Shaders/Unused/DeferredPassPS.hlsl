
#include "../Globals/GlobalsPS.hlsli"



#if SHADOWS
#include "../Util/ShadowUtil.hlsli"
#endif


//gbuffer
Texture2D normalTx          : register(t0);
Texture2D diffuseTx         : register(t1);
Texture2D<float> depthTx    : register(t2);




#if SHADOWS
Texture2D       shadowDepthMap  : register(t4);
TextureCube     depthCubeMap    : register(t5);
Texture2DArray  cascadeDepthMap : register(t6);
#endif






struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 Tex  : TEX;
};


float4 ps_main(VertexOut pin) : SV_TARGET
{

    //unpack gbuffer
    float4 NormalSpecIntensity = normalTx.Sample(linear_wrap_sampler, pin.Tex);
    float3 Normal = 2 * NormalSpecIntensity.rgb - 1.0;
    
    //return float4(Normal, 1.0f);
    float4 diffuseSpecPower = diffuseTx.Sample(linear_wrap_sampler, pin.Tex);
    
    float shininess = pow(2, diffuseSpecPower.a * 10.5f);
    
    float depth = depthTx.Sample(linear_wrap_sampler, pin.Tex);
    
    float3 Position = GetPositionVS(pin.Tex, depth);
    
    

    float3 V = normalize(eye_position.xyz - Position);
   
    LightingResult lr;
    switch (current_light.type)
    {
        case DIRECTIONAL_LIGHT:
            lr = DoDirectionalLight(current_light, shininess, V, Normal);
            break;
        case POINT_LIGHT:
            lr = DoPointLight(current_light, shininess, V, Position, Normal);
            break;
        case SPOT_LIGHT:
            lr = DoSpotLight(current_light, shininess, V, Position, Normal);
            break;
        default:
            return float4(0, 1, 0, 1);
    }
    

    float3 D = lr.Diffuse.xyz; 
    float3 S = lr.Specular.xyz;
    
#if SHADOWS
    if (current_light.casts_shadows)
    {

        float shadow_factor = 1.0f;
        if (current_light.type == POINT_LIGHT)
        {
            const float zf = current_light.range;
            const float zn = 0.5f;
            const float c1 = zf / (zf - zn);
            const float c0 = -zn * zf / (zf - zn);
            
            float3 light_to_pixelWS = mul(float4(Position - current_light.position.xyz, 0.0f), inverse_view);

            const float3 m = abs(light_to_pixelWS).xyz;
            const float major = max(m.x, max(m.y, m.z));
            float fragment_depth = (c1 * major + c0) / major;
            shadow_factor = depthCubeMap.SampleCmpLevelZero(shadow_sampler, normalize(light_to_pixelWS.xyz), fragment_depth);
            
        }
        else if (current_light.type == DIRECTIONAL_LIGHT && current_light.use_cascades)
        {
            float viewDepth = Position.z;
            for (uint i = 0; i < 3; ++i)
            {
                matrix light_space_matrix = i == 0 ? shadow_matrix : i == 1 ? shadow_matrix2 : shadow_matrix3;
                    
                float4 posShadowMap = mul(float4(Position, 1.0), light_space_matrix);
        
                float3 UVD = posShadowMap.xyz / posShadowMap.w;

                UVD.xy = 0.5 * UVD.xy + 0.5;
                UVD.y = 1.0 - UVD.y;

                if (viewDepth < splits[i])
                {
                    shadow_factor = CSMCalcShadowFactor_PCF3x3(shadow_sampler, cascadeDepthMap, i, UVD, shadow_map_size, softness);
                      
                    break;
                }
            }    
        }
        else
        {
            float4 posShadowMap = mul(float4(Position, 1.0), shadow_matrix);
            float3 UVD = posShadowMap.xyz / posShadowMap.w;

            UVD.xy = 0.5 * UVD.xy + 0.5;
            UVD.y = 1.0 - UVD.y;
                
            shadow_factor = CalcShadowFactor_PCF3x3(shadow_sampler, shadowDepthMap, UVD, shadow_map_size, softness);
        }

        D = D * shadow_factor;
        S = S * shadow_factor;
    }
    
#endif

    float4 finalColor = float4(diffuseSpecPower.rgb * D + NormalSpecIntensity.a * S, 1.0);
   
    return finalColor;
}