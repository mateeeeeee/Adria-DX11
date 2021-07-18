#pragma once
#include "../Core/Definitions.h"



namespace adria
{
	
#define DECLARE_CBUFFER_SLOT(name, slot) static constexpr u16 CBUFFER_SLOT_##name = slot
#define DECLARE_TEXTURE_SLOT(name, slot) static constexpr u16 TEXTURE_SLOT_##name = slot

	DECLARE_CBUFFER_SLOT(FRAME, 0);
	DECLARE_CBUFFER_SLOT(OBJECT, 1);
	DECLARE_CBUFFER_SLOT(LIGHT, 2);
	DECLARE_CBUFFER_SLOT(SHADOW, 3);
	DECLARE_CBUFFER_SLOT(MATERIAL, 4);
	DECLARE_CBUFFER_SLOT(POSTPROCESS, 5);
	DECLARE_CBUFFER_SLOT(COMPUTE, 6);
	DECLARE_CBUFFER_SLOT(WEATHER, 7);
	DECLARE_CBUFFER_SLOT(VOXEL, 8);

	DECLARE_TEXTURE_SLOT(DIFFUSE, 0);
	DECLARE_TEXTURE_SLOT(SPECULAR, 1);
	DECLARE_TEXTURE_SLOT(ROUGHNESS_METALLIC, 1);
	DECLARE_TEXTURE_SLOT(NORMAL, 2);
	DECLARE_TEXTURE_SLOT(EMISSIVE, 3);
	DECLARE_TEXTURE_SLOT(METALLIC, 7);
	DECLARE_TEXTURE_SLOT(ROUGHNESS, 8);
	DECLARE_TEXTURE_SLOT(CUBEMAP, 0);
	DECLARE_TEXTURE_SLOT(SHADOW, 4);
	DECLARE_TEXTURE_SLOT(SHADOWCUBE, 5);
	DECLARE_TEXTURE_SLOT(SHADOWARRAY, 6);

	//terrain textures
	DECLARE_TEXTURE_SLOT(GRASS, 0);
	DECLARE_TEXTURE_SLOT(SNOW, 1);
	DECLARE_TEXTURE_SLOT(ROCK, 2);
	DECLARE_TEXTURE_SLOT(SAND, 3);
	
	enum class StandardShader : u8
	{
		eSkybox,
		eTexture,
		eSolid,
		eSun,
		eBillboard,
		eGbufferPBR,
		eGBufferPBR_Separated,
		eAmbientPBR,
		eAmbientPBR_AO,
		eAmbientPBR_IBL,
		eAmbientPBR_AO_IBL,
		eLightingPBR,
		eClusterLightingPBR,
		eToneMap_Reinhard,
		eToneMap_Hable,
		eToneMap_Linear,
		eFXAA,
		eTAA,
		eCopy,
		eAdd,
		eDepthMap,
		eDepthMap_Transparent,
		eVolumetric_Directional,
		eVolumetric_DirectionalCascades,
		eVolumetric_Spot,
		eVolumetric_Point,
		eVolumetric_Clouds,
		eSSAO,
		eSSR,
		eHBAO,
		eGodRays,
		eMotionBlur,
		eDof,
		eFog,
		eOcean,
		eVoxelGI,
        eUnknown
	};

	enum class ComputeShader : u8
	{
		eBlur_Horizontal,
		eBlur_Vertical,
		eBloomExtract,
		eBloomCombine,
		eBokehGenerate,
		eOceanInitialSpectrum,
		eOceanSpectrum,
		eOceanFFT_Horizontal,
		eOceanFFT_Vertical,
		eOceanNormalMap,
		eOceanPhase,
		eTiledLighting,
		eClusterBuilding,
		eClusterCulling,
		eVoxelCopy,
		eVoxelSecondBounce
	};

	enum class GeometryShader : u8
	{
		eLensFlare,
		eBokehDraw,
		eVoxelize,
		eVoxelizeDebug
	};

	enum class TesselationShader : u8
	{
		eOcean
	};

	enum class ToneMap : u8
	{
		eReinhard,
		eHable,
		eLinear
	};

	enum class LightType : i32
	{
		eDirectional,
		ePoint,
		eSpot
	};

	enum class FogType : i32
	{
		eExponential, 
		eExponentialHeight
	};

	enum class AmbientOclussion : u8
	{
		eNone,
		eSSAO,
		eHBAO
	};

	enum class BokehType : u8
	{
		eHex,
		eOct,
		eCircle,
		eCross
	};

	enum AntiAliasing : u8
	{
		AntiAliasing_None = 0x0,
		AntiAliasing_FXAA = 0x1,
		AntiAliasing_TAA = 0x2
	};

}
