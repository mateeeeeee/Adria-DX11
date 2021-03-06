#include "../Globals/GlobalsPS.hlsli"

#include "../Util/ShadowUtil.hlsli"

struct PS_INPUT
{
    float4 Position     : SV_POSITION;
    float4 PositionWS   : POSITION;
    float2 Uvs          : TEX;
    float3 NormalWS     : NORMAL0;
};

RWStructuredBuffer<VoxelType> VoxelGrid     : register(u0);

Texture2D albedoTx                          : register(t0);

Texture2D shadowMap                         : register(t4);

Texture2DArray cascadeShadowMaps            : register(t6);

StructuredBuffer<StructuredLight> lights    : register(t10);



void main(PS_INPUT input)
{
    
    float3 diff = (input.PositionWS.xyz - voxel_radiance.GridCenter) * voxel_radiance.DataResRCP * voxel_radiance.DataSizeRCP;
    float3 uvw = diff * float3(0.5f, -0.5f, 0.5f) + 0.5f;

    [branch]
    if (any(0 > uvw || 1 <= uvw))
    {
        return;
    }
    
    float3 normal = normalize(input.NormalWS);

    input.Uvs.y = 1 - input.Uvs.y;
    float4 color = albedoTx.Sample(linear_wrap_sampler, input.Uvs) * albedo_factor;

    uint light_count, _dummy;
    lights.GetDimensions(light_count, _dummy);
    light_count = min(light_count, max_voxel_lights); 
    
    float4 lighting = 0;
    for (int i = 0; i < light_count; ++i)
    {
        
        if (!lights[i].active)
            continue;

        Light light = CreateLightFromStructured(lights[i]);

        switch (light.type)
        {
            case DIRECTIONAL_LIGHT:
                float4 diffuse = DoDiffuse(light, normalize(-light.direction.xyz), normal);
                if (light.casts_shadows)
                {
                    float4 PosVS = mul(input.PositionWS, view);
                    PosVS /= PosVS.w;
                    if (light.use_cascades)
                    {
                        float viewDepth = PosVS.z;
                    
                        matrix shadow_matrices[3] = { shadow_matrix1, shadow_matrix2, shadow_matrix3 };

                        for (uint i = 0; i < 3; ++i)
                        {
                            matrix light_space_matrix = shadow_matrices[i];
                    
                            if (viewDepth < splits[i])
                            {
                                float4 posShadowMap = mul(float4(PosVS.xyz, 1.0), light_space_matrix);
        
                                float3 UVD = posShadowMap.xyz / posShadowMap.w;

                                UVD.xy = 0.5 * UVD.xy + 0.5;
                                UVD.y = 1.0 - UVD.y;

                                diffuse *= CSMCalcShadowFactor_PCF3x3(shadow_sampler, cascadeShadowMaps, i, UVD, shadow_map_size, softness);
                                break;
                            }
                        }
                    }
                    else
                    {
                        float4 posShadowMap = mul(float4(PosVS.xyz, 1.0), shadow_matrix1);
                        float3 UVD = posShadowMap.xyz / posShadowMap.w;

                        UVD.xy = 0.5 * UVD.xy + 0.5;
                        UVD.y = 1.0 - UVD.y;
                        diffuse *= CalcShadowFactor_PCF3x3(shadow_sampler, shadowMap, UVD, shadow_map_size, softness);
                    }
                }
        
                lighting += diffuse;
                break;
            
        case POINT_LIGHT:
                
                light.position.xyz /= light.position.w;
                float3 L_ = light.position.xyz - input.PositionWS.xyz;
                float distance_ = length(L_);
                L_ = L_ / distance_;
                float attenuation_ = DoAttenuation(distance_, light.range);
                if (attenuation_ < 0.0001f) break;
                lighting += DoDiffuse(light, L_, normal) * attenuation_;
                break;
        case SPOT_LIGHT:          
                float3 L = light.position.xyz - input.PositionWS.xyz;
                float distance = length(L);
                L /= distance;
                float attenuation = DoAttenuation(distance, light.range);
                if (attenuation < 0.0001f) break;
                float3 normalized_light_dir = normalize(light.direction.xyz);
                float cosAng = dot(-normalized_light_dir, L);
                float conAtt = saturate((cosAng - light.inner_cosine) / (light.outer_cosine - light.inner_cosine));
                conAtt *= conAtt;
                lighting += DoDiffuse(light, L, normal) * attenuation * conAtt;
                break;
            
        default:
                lighting += 0;
                break;
        }
    }
    
    color *= (lighting + global_ambient);

    uint colorEncoded = EncodeColor(color);
    uint normalEncoded = EncodeNormal(normal);
    
    uint3 writecoord = floor(uvw * voxel_radiance.DataRes);
    uint id = Flatten3D(writecoord, voxel_radiance.DataRes);
    
    
    InterlockedMax(VoxelGrid[id].ColorMask, colorEncoded);
    InterlockedMax(VoxelGrid[id].NormalMask, normalEncoded);
}