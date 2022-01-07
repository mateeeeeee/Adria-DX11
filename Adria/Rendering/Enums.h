#pragma once
#include "../Core/Definitions.h"



namespace adria
{
	
#define DECLARE_CBUFFER_SLOT(name, slot) static constexpr U16 CBUFFER_SLOT_##name = slot
#define DECLARE_TEXTURE_SLOT(name, slot) static constexpr U16 TEXTURE_SLOT_##name = slot

	DECLARE_CBUFFER_SLOT(FRAME, 0);
	DECLARE_CBUFFER_SLOT(OBJECT, 1);
	DECLARE_CBUFFER_SLOT(LIGHT, 2);
	DECLARE_CBUFFER_SLOT(SHADOW, 3);
	DECLARE_CBUFFER_SLOT(MATERIAL, 4);
	DECLARE_CBUFFER_SLOT(POSTPROCESS, 5);
	DECLARE_CBUFFER_SLOT(COMPUTE, 6);
	DECLARE_CBUFFER_SLOT(WEATHER, 7);
	DECLARE_CBUFFER_SLOT(VOXEL, 8);
	DECLARE_CBUFFER_SLOT(TERRAIN, 9);

	DECLARE_TEXTURE_SLOT(DIFFUSE, 0);
	DECLARE_TEXTURE_SLOT(SPECULAR, 1);
	DECLARE_TEXTURE_SLOT(ROUGHNESS_METALLIC, 1);
	DECLARE_TEXTURE_SLOT(NORMAL, 2);
	DECLARE_TEXTURE_SLOT(EMISSIVE, 3);
	DECLARE_TEXTURE_SLOT(CUBEMAP, 0);
	DECLARE_TEXTURE_SLOT(SHADOW, 4);
	DECLARE_TEXTURE_SLOT(SHADOWCUBE, 5);
	DECLARE_TEXTURE_SLOT(SHADOWARRAY, 6);

	//terrain textures
	DECLARE_TEXTURE_SLOT(GRASS, 0);
	DECLARE_TEXTURE_SLOT(BASE, 1);
	DECLARE_TEXTURE_SLOT(ROCK, 2);
	DECLARE_TEXTURE_SLOT(SAND, 3);
	DECLARE_TEXTURE_SLOT(LAYER, 4);
	
	enum class EShader : U8
	{
		Skybox,
		UniformColorSky,
		HosekWilkieSky,
		Texture,
		Solid,
		Sun,
		Billboard,
		GBufferPBR,
		GBuffer_Terrain,
		AmbientPBR,
		AmbientPBR_AO,
		AmbientPBR_IBL,
		AmbientPBR_AO_IBL,
		LightingPBR,
		ClusterLightingPBR,
		ToneMap_Reinhard,
		ToneMap_Hable,
		ToneMap_Linear,
		FXAA,
		TAA,
		Copy,
		Add,
		DepthMap,
		DepthMap_Transparent,
		Volumetric_Directional,
		Volumetric_DirectionalCascades,
		Volumetric_Spot,
		Volumetric_Point,
		Volumetric_Clouds,
		SSAO,
		SSR,
		HBAO,
		GodRays,
		MotionBlur,
		DOF,
		Fog,
		Ocean,
		VoxelGI,
		VelocityBuffer,
		GBuffer_Foliage,
		Particles,
        Unknown
	};

	enum class EComputeShader : U8
	{
		Blur_Horizontal,
		Blur_Vertical,
		BloomExtract,
		BloomCombine,
		BokehGenerate,
		OceanInitialSpectrum,
		OceanSpectrum,
		OceanFFT_Horizontal,
		OceanFFT_Vertical,
		OceanNormalMap,
		OceanPhase,
		TiledLighting,
		ClusterBuilding,
		ClusterCulling,
		VoxelCopy,
		VoxelSecondBounce,
		ParticleInitDeadList,
		ParticleReset,
		ParticleEmit,
		ParticleSimulate,
		ParticleBitonicSortStep,
		ParticleSort512,
		ParticleSortInner512,
		ParticleSortInitArgs
	};

	enum class EGeometryShader : U8
	{
		LensFlare,
		BokehDraw,
		Voxelize,
		VoxelizeDebug
	};

	enum class ETesselationShader : U8
	{
		Ocean
	};

	enum class EToneMap : U8
	{
		Reinhard,
		Hable,
		Linear
	};

	enum class ELightType : I32
	{
		Directional,
		Point,
		Spot
	};

	enum class EFogType : I32
	{
		Exponential, 
		ExponentialHeight
	};

	enum class EAmbientOcclusion : U8
	{
		None,
		SSAO,
		HBAO
	};

	enum class EBokehType : U8
	{
		Hex,
		Oct,
		Circle,
		Cross
	};

	enum class ESkyType : U8
	{
		UniformColor,
		Skybox,
		HosekWilkie
	};


	enum class EBlendState
	{
		None,
		AlphaToCoverage,
		AdditiveBlend,
		AlphaBlend
	};

	enum class EDepthState
	{
		None
	};

	enum class ERasterizerState
	{
		None
	};

	enum EAntiAliasing : U8
	{
		EAntiAliasing_None = 0x0,
		EAntiAliasing_FXAA = 0x1,
		EAntiAliasing_TAA = 0x2
	};

	enum EGBufferSlot : U8
	{
		EGBufferSlot_NormalMetallic,
		EGBufferSlot_DiffuseRoughness,
		EGBufferSlot_Emissive,
		EGBufferSlot_Count
	};
}
