#include "../Globals/GlobalsPS.hlsli"
#include "../Util/DitherUtil.hlsli"
//credits: https://github.com/martinsh/godot-SSGI

static const int SAMPLES = 16; //8-32


//in postprocess_cbuf add: int noise,float noise_amount, indirect amount
static const int noise = true;
static const float noise_amount = 1; //0-5
static const float indirect_amount = 3; //0-512

Texture2D scene_texture : register(t0);
Texture2D normal_texture : register(t1);
Texture2D depth_texture : register(t2);

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 Tex : TEX;
};

float3 LightSample(float2 coord, float2 lightcoord, float3 normal_vs, float3 position_vs, float n, float2 dims)
{
    float2 random = 1.0;
    if (noise)
    {
        random = ( mod_dither((coord * dims) + float2(n * 82.294, n * 127.721))) * 0.01 * noise_amount;
    }
    else
    {
        random = dither(coord, 1.0, dims) * 0.1 * noise_amount;
    }
    lightcoord *= 0.7f;

    float depth = depth_texture.Sample(linear_wrap_sampler, lightcoord + random);
    
    float3 light_pos_vs = GetPositionVS(lightcoord + random, depth);
    float4 Normal = normal_texture.Sample(linear_wrap_sampler, lightcoord + random).rgb;
    float3 light_normal_vs = 2 * Normal - 1.0;
    float3 light_color = scene_texture.SampleLevel(linear_wrap_sampler, lightcoord + random, 4.0f);
    //light variable data
    float3 lightpath = light_pos_vs - position_vs;
    float3 lightdir = normalize(lightpath);
    
    //falloff calculations
    float cosemit = clamp(dot(lightdir, -light_normal_vs), 0.0, 1.0); //emit only in one direction
    float coscatch = clamp(dot(lightdir, normal_vs) * 0.5 + 0.5, 0.0, 1.0); //recieve light from one direction
    float distfall = pow(dot(lightpath, lightpath), 0.1) + 1.0; //fall off with distance
    
    return (light_color * cosemit * coscatch / distfall) * (length(light_pos_vs) / 20.0);
}

float4 main(VertexOut pin) : SV_TARGET
{
    return 0;
   
    float3 direct_color = scene_texture.SampleLevel(linear_wrap_sampler, pin.Tex, 0).rgb;
	
    float3 indirect_color = 0.0f;
    
    uint width, height, dummy;
    scene_texture.GetDimensions(0, width, height, dummy);
    float2 dims = float2(width, height);
	
    float depth = depth_texture.SampleLevel(linear_wrap_sampler, pin.Tex, 0).r;
    
    float3 posVS = GetPositionVS(pin.Tex, depth);
    float4 Normal = normal_texture.SampleLevel(linear_wrap_sampler, pin.Tex, 0).rgb;
    float3 normalVS = 2 * Normal - 1.0;
    
     //sampling in spiral
    
    float dlong = PI * (3.0 - sqrt(5.0));
    float dz = 1.0 / float(SAMPLES);
    float long = 0.0;
    float z = 1.0 - dz / 2.0;
    
    for (int i = 0; i < SAMPLES; i++)
    {
            
        float r = sqrt(1.0 - z);
        
        float xpoint = (cos(long) * r) * 0.5 + 0.5;
        float ypoint = (sin(long) * r) * 0.5 + 0.5;
                
        z = z - dz;
        long = long + dlong;
    
        indirect_color += LightSample(pin.Tex, float2(xpoint, ypoint), normalVS, posVS, float(i), dims);

    }

	
    return direct_color + (indirect_color / float(SAMPLES) * indirect_amount);
}