#ifndef _COMMON_DATA_
#define _COMMON_DATA_

static const int SSAO_KERNEL_SIZE = 16;

struct FrameData
{
    float4 globalAmbient;
    row_major matrix view;
    row_major matrix projection;
    row_major matrix viewprojection;
    row_major matrix inverseView;
    row_major matrix inverseProjection;
    row_major matrix inverseViewProjection;
    row_major matrix prevView;
    row_major matrix prevProjection;
    row_major matrix prevViewProjection;
    float4 cameraPosition;
    float4 cameraForward;
    float  cameraNear;
    float  cameraFar;
    float2 screenResolution;
    float2 mouseNormalizedCoords;
};
struct ObjectData
{
    row_major matrix model;
    row_major matrix transposedInverseModel;
};

struct ShadowData
{
    row_major matrix lightViewProjection;
    row_major matrix lightView;
    row_major matrix shadowMatrices[4];
    float split0;
    float split1;
    float split3;
    float softness;
    int shadowMapSize;
    int visualize;
};

struct WeatherData
{
    float4 lightDir;
    float4 lightColor;
    float4 skyColor;
    float4 ambientColor;
    float4 windDir;
    
    float windSpeed;
    float time;
    float crispiness;
    float curliness;
    
    float coverage;
    float absorption;
    float cloudsBottomHeight;
    float cloudsTopHeight;
    
    float densityFactor;
    float cloudType;
    //padd float2

    float3 A;
    float3 B;
    float3 C;
    float3 D;
    float3 E;
    float3 F;
    float3 G;
    float3 H;
    float3 I;
    float3 Z;
};

struct MaterialData
{
    float3 ambient;
    float3 diffuse;
    float  alphaCutoff;
    float3 specular;
    float  shininess;
    
    float albedoFactor;
    float metallicFactor;
    float roughnessFactor;
    float emissiveFactor;
};

struct PostprocessData
{
    float2  ssaoNoiseScale;
    float   ssaoRadius;
    float   ssaoPower;
    float4  ssaoSamples[SSAO_KERNEL_SIZE];
    float   ssrRayStep;
    float   ssrRayHitThreshold;
    float   velocityBufferScale;
    float   toneMapExposure;
    float4  dofParams;
    float4  fogColor;
    float   fogFalloff;
    float   fogDensity;
    float   fogStart;
    int     fogType;
    float   hbaoR2;
    float   hbaoRadiusToScreen;
    float   hbaoPower;
};

struct ComputeData
{
    float bloomScale; 
    float threshold;  
    
    float gaussCoeff1;
    float gaussCoeff2;
    float gaussCoeff3;
    float gaussCoeff4;
    float gaussCoeff5;
    float gaussCoeff6;
    float gaussCoeff7;
    float gaussCoeff8;
    float gaussCoeff9;
    
    float  bokehFallout;      
    float4 dofParams;         
    float  bokehRadiusScale;  
    float  bokehColorScale;   
    float  bokehBlurThreshold;
    float  bokehLumThreshold; 
    
    int oceanSize;           
    int resolution;          
    float oceanChoppiness;   		
    float windDirectionX;    
    float windDirectionY;    
    float deltaTime;         
    int visualizeTiled;      
    int lightsCountWhite;
}


#endif //_COMMON_DATA_