
#include <unordered_map>
#include <memory>
#include <string_view>
#include <execution>
#include <filesystem>
#include "ShaderCache.h"
#include "../Graphics/ShaderProgram.h"
#include "../Graphics/ShaderCompiler.h"
#include "../Logging/Logger.h"
#include "../Utilities/Timer.h"

namespace fs = std::filesystem;

namespace adria
{
	namespace
	{
		template<typename T>
		inline void FreeContainer(T& container)
		{
			T empty;
			using std::swap;
			swap(container, empty);
		}

		struct ShaderFileData
		{
			fs::file_time_type last_changed_timestamp;
		};

		ID3D11Device* device;
		std::unordered_map<EShader, ShaderBlob> shader_map;
		std::unordered_map<EShader, ShaderFileData> shader_file_data_map;
		std::unordered_map<EShaderProgram, std::unique_ptr<ShaderProgram>> shader_program_map;
		std::unordered_map<EShaderProgram, std::vector<EShader>> dependency_map;

		std::string const compiled_shaders_directory = "Resources/Compiled Shaders/";
		std::string const shaders_directory = "Resources/Shaders/";
		std::string const shaders_headers_directories[] = { "Resources/Shaders/Globals", "Resources/Shaders/Util/" };

		constexpr EShaderStage GetStage(EShader shader)
		{
			switch (shader)
			{
			case VS_Sky:
			case VS_Texture:
			case VS_Solid:
			case VS_Billboard:
			case VS_Sun:
			case VS_Decals:
			case VS_GBufferTerrain:
			case VS_GBufferPBR:
			case VS_ScreenQuad:
			case VS_LensFlare:
			case VS_Bokeh:
			case VS_DepthMap:
			case VS_DepthMapTransparent:
			case VS_Ocean:
			case VS_OceanLOD:
			case VS_Voxelize:
			case VS_VoxelizeDebug:
			case VS_Foliage:
			case VS_Particles:
				return EShaderStage::VS;
			case PS_Skybox:
			case PS_HosekWilkieSky:
			case PS_UniformColorSky:
			case PS_Texture:
			case PS_Solid:
			case PS_Sun:
			case PS_Billboard:
			case PS_Decals:
			case PS_DecalsModifyNormals:
			case PS_GBufferPBR:
			case PS_GBufferTerrain:
			case PS_AmbientPBR:
			case PS_AmbientPBR_AO:
			case PS_AmbientPBR_IBL:
			case PS_AmbientPBR_AO_IBL:
			case PS_LightingPBR:
			case PS_ClusterLightingPBR:
			case PS_ToneMap_Reinhard:
			case PS_ToneMap_Linear:
			case PS_ToneMap_Hable:
			case PS_FXAA:
			case PS_TAA:
			case PS_Copy:
			case PS_Add:
			case PS_SSAO:
			case PS_HBAO:
			case PS_SSR:
			case PS_LensFlare:
			case PS_GodRays:
			case PS_DepthOfField:
			case PS_Bokeh:
			case PS_VolumetricClouds:
			case PS_VelocityBuffer:
			case PS_MotionBlur:
			case PS_Fog:
			case PS_DepthMap:
			case PS_DepthMapTransparent:
			case PS_VolumetricLight_Directional:
			case PS_VolumetricLight_Spot:
			case PS_VolumetricLight_Point:
			case PS_VolumetricLight_DirectionalWithCascades:
			case PS_Ocean:
			case PS_OceanLOD:
			case PS_VoxelGI:
			case PS_Voxelize:
			case PS_VoxelizeDebug:
			case PS_Foliage:
			case PS_Particles:
				return EShaderStage::PS;
			case GS_LensFlare:
			case GS_Bokeh:
			case GS_Voxelize:
			case GS_VoxelizeDebug:
				return EShaderStage::GS;
			case CS_BlurHorizontal:
			case CS_BlurVertical:
			case CS_BokehGenerate:
			case CS_BloomExtract:
			case CS_BloomCombine:
			case CS_OceanInitialSpectrum:
			case CS_OceanPhase:
			case CS_OceanSpectrum:
			case CS_OceanFFT_Horizontal:
			case CS_OceanFFT_Vertical:
			case CS_OceanNormalMap:
			case CS_TiledLighting:
			case CS_ClusterBuilding:
			case CS_ClusterCulling:
			case CS_VoxelCopy:
			case CS_VoxelSecondBounce:
			case CS_ParticleInitDeadList:
			case CS_ParticleReset:
			case CS_ParticleEmit:
			case CS_ParticleSimulate:
			case CS_ParticleBitonicSortStep:
			case CS_ParticleSort512:
			case CS_ParticleSortInner512:
			case CS_ParticleSortInitArgs:
			case CS_Picker:
				return EShaderStage::CS;
			case HS_OceanLOD:
				return EShaderStage::HS;
			case DS_OceanLOD:
				return EShaderStage::DS;
			case EShader_Count:
			default:
				return EShaderStage::STAGE_COUNT;
			}
		}
		constexpr std::string GetShaderSource(EShader shader)
		{
			switch (shader)
			{
			case VS_Sky:
				return "Misc/SkyboxVS.hlsl";
			case PS_Skybox:
				return "Misc/SkyboxPS.hlsl";
			case PS_HosekWilkieSky:
				return "Misc/HosekWilkieSkyPS.hlsl";
			case PS_UniformColorSky:
				return "Misc/UniformColorSkyPS.hlsl";
			case VS_Texture:
				return "Misc/TextureVS.hlsl";
			case PS_Texture:
				return "Misc/TexturePS.hlsl";
			case VS_Solid:
				return "Misc/SolidVS.hlsl";
			case PS_Solid:
				return "Misc/SolidPS.hlsl";
			case VS_Sun:
				return "Misc/SunVS.hlsl";
			case PS_Sun:
				return "Misc/SunPS.hlsl";
			case VS_Billboard:
				return "Misc/BillboardVS.hlsl";
			case PS_Billboard:
				return "Misc/BillboardPS.hlsl";
			case VS_Decals:
				return "Misc/DecalVS.hlsl";
			case PS_Decals:
			case PS_DecalsModifyNormals:
				return "Misc/DecalPS.hlsl";
			case VS_GBufferPBR:
				return "Deferred/GeometryPassPBR_VS.hlsl";
			case PS_GBufferPBR:
				return "Deferred/GeometryPassPBR_PS.hlsl";
			case VS_GBufferTerrain:
				return "Deferred/GeometryPassTerrain_VS.hlsl";
			case PS_GBufferTerrain:
				return "Deferred/GeometryPassTerrain_PS.hlsl";
			case VS_ScreenQuad:
				return "Postprocess/ScreenQuadVS.hlsl";
			case PS_AmbientPBR:
			case PS_AmbientPBR_AO:
			case PS_AmbientPBR_IBL:
			case PS_AmbientPBR_AO_IBL:
				return "Deferred/AmbientPBR_PS.hlsl";
			case PS_LightingPBR:
				return "Deferred/LightingPBR_PS.hlsl";
			case PS_ClusterLightingPBR:
				return "Deferred/ClusterLightingPBR_PS.hlsl";
			case PS_ToneMap_Reinhard:
			case PS_ToneMap_Linear:
			case PS_ToneMap_Hable:
				return "Postprocess/ToneMapPS.hlsl";
			case PS_FXAA:
				return "Postprocess/FXAA.hlsl";
			case PS_TAA:
				return "Postprocess/TAA_PS.hlsl";
			case PS_Copy:
				return "Postprocess/CopyPS.hlsl";
			case PS_Add:
				return "Postprocess/AddPS.hlsl";
			case PS_SSAO:
				return "Postprocess/SSAO_PS.hlsl";
			case PS_HBAO:
				return "Postprocess/HBAO_PS.hlsl";
			case PS_SSR:
				return "Postprocess/SSR_PS.hlsl";
			case VS_LensFlare:
				return "Postprocess/LensFlareVS.hlsl";
			case GS_LensFlare:
				return "Postprocess/LensFlareGS.hlsl";
			case PS_LensFlare:
				return "Postprocess/LensFlarePS.hlsl";
			case PS_GodRays:
				return "Postprocess/GodRaysPS.hlsl";
			case PS_DepthOfField:
				return "Postprocess/DOF_PS.hlsl";
			case VS_Bokeh:
				return "Postprocess/BokehVS.hlsl";
			case GS_Bokeh:
				return "Postprocess/BokehGS.hlsl";
			case PS_Bokeh:
				return "Postprocess/BokehPS.hlsl";
			case PS_VolumetricClouds:
				return "Postprocess/CloudsPS.hlsl";
			case PS_VelocityBuffer:
				return "Postprocess/VelocityBufferPS.hlsl";
			case PS_MotionBlur:
				return "Postprocess/MotionBlurPS.hlsl";
			case PS_Fog:
				return "Postprocess/FogPS.hlsl";
			case VS_DepthMap:
			case VS_DepthMapTransparent:
				return "Shadows/DepthMapVS.hlsl";
			case PS_DepthMap:
			case PS_DepthMapTransparent:
				return "Shadows/DepthMapPS.hlsl";
			case PS_VolumetricLight_Directional:
				return "Postprocess/VolumetricLightDirectionalPS.hlsl";
			case PS_VolumetricLight_DirectionalWithCascades:
				return "Postprocess/VolumetricLightDirectionalCascadesPS.hlsl";
			case PS_VolumetricLight_Spot:
				return "Postprocess/VolumetricLightSpotPS.hlsl";
			case PS_VolumetricLight_Point:
				return "Postprocess/VolumetricLightPointPS.hlsl";
			case CS_BlurHorizontal:
			case CS_BlurVertical:
				return "Postprocess/BlurCS.hlsl";
			case CS_BokehGenerate:
				return "Postprocess/BokehCS.hlsl";
			case CS_BloomExtract:
				return "Postprocess/BloomExtractCS.hlsl";
			case CS_BloomCombine:
				return "Postprocess/BloomCombineCS.hlsl";
			case CS_OceanInitialSpectrum:
				return "Ocean/InitialSpectrumCS.hlsl";
			case CS_OceanPhase:
				return "Ocean/PhaseCS.hlsl";
			case CS_OceanSpectrum:
				return "Ocean/SpectrumCS.hlsl";
			case CS_OceanFFT_Horizontal:
				return "Ocean/FFT_horizontalCS.hlsl";
			case CS_OceanFFT_Vertical:
				return "Ocean/FFT_verticalCS.hlsl";
			case CS_OceanNormalMap:
				return "Ocean/NormalMapCS.hlsl";
			case CS_TiledLighting:
				return "Deferred/TiledLightingCS.hlsl";
			case CS_ClusterBuilding:
				return "Deferred/ClusterBuildingCS.hlsl";
			case CS_ClusterCulling:
				return "Deferred/ClusterCullingCS.hlsl";
			case CS_VoxelCopy:
				return "GI/VoxelCopyCS.hlsl";
			case CS_VoxelSecondBounce:
				return "GI/VoxelSecondBounceCS.hlsl";
			case VS_Ocean:
				return "Ocean/OceanVS.hlsl";
			case PS_Ocean:
				return "Ocean/OceanPS.hlsl";
			case VS_OceanLOD:
				return "Ocean/OceanLodVS.hlsl";
			case HS_OceanLOD:
				return "Ocean/OceanLodHS.hlsl";
			case DS_OceanLOD:
				return "Ocean/OceanLodDS.hlsl";
			case PS_OceanLOD:
				return "Ocean/OceanLodPS.hlsl";
			case VS_Voxelize:
				return "GI/VoxelizeVS.hlsl";
			case GS_Voxelize:
				return "GI/VoxelizeGS.hlsl";
			case PS_Voxelize:
				return "GI/VoxelizePS.hlsl";
			case VS_VoxelizeDebug:
				return "GI/VoxelDebugVS.hlsl";
			case GS_VoxelizeDebug:
				return "GI/VoxelDebugGS.hlsl";
			case PS_VoxelizeDebug:
				return "GI/VoxelDebugPS.hlsl";
			case PS_VoxelGI:
				return "GI/VoxelGI_PS.hlsl";
			case VS_Foliage:
				return "Misc/FoliageVS.hlsl";
			case PS_Foliage:
				return "Misc/FoliagePS.hlsl";
			case CS_Picker:
				return "Misc/PickerCS.hlsl";
			case VS_Particles:
				return "Particles/ParticleVS.hlsl";
			case PS_Particles:
				return "Particles/ParticlePS.hlsl";
			case CS_ParticleInitDeadList:
				return "Particles/InitDeadListCS.hlsl";
			case CS_ParticleReset:
				return "Particles/ParticleResetCS.hlsl";
			case CS_ParticleEmit:
				return "Particles/ParticleEmitCS.hlsl";
			case CS_ParticleSimulate:
				return "Particles/ParticleSimulateCS.hlsl";
			case CS_ParticleBitonicSortStep:
				return "Particles/BitonicSortStepCS.hlsl";
			case CS_ParticleSort512:
				return "Particles/Sort512CS.hlsl";
			case CS_ParticleSortInner512:
				return "Particles/SortInner512CS.hlsl";
			case CS_ParticleSortInitArgs:
				return "Particles/InitSortDispatchArgsCS.hlsl";
			case EShader_Count:
			default:
				return "";
			}
		}
		constexpr std::vector<ShaderMacro> GetShaderMacros(EShader shader)
		{
			switch (shader)
			{
			case PS_DecalsModifyNormals:
				return { {"DECAL_MODIFY_NORMALS", ""} };
			case PS_AmbientPBR_AO:
				return { {"SSAO", "1"} };
			case PS_AmbientPBR_IBL:
				return { {"IBL", "1"} };
			case PS_AmbientPBR_AO_IBL:
				return { {"SSAO", "1"}, {"IBL", "1"} };
			case PS_ToneMap_Reinhard:
				return { {"REINHARD", "1" } };
			case PS_ToneMap_Linear:
				return { {"LINEAR", "1" } };
			case PS_ToneMap_Hable:
				return { {"HABLE", "1" } };
			case VS_DepthMapTransparent:
			case PS_DepthMapTransparent:
				return { {"TRANSPARENT", "1"} };
			case CS_BlurVertical:
				return { { "VERTICAL", "1" } };
			default:
				return {};
			}
		}

		void AddDependency(EShaderProgram program, std::vector<EShader> const& shaders)
		{
			dependency_map[program] = shaders;
		}
		void CreateAllPrograms()
		{
			shader_program_map[EShaderProgram::Skybox] = std::make_unique<StandardProgram>(device, shader_map[VS_Sky], shader_map[PS_Skybox]); AddDependency(EShaderProgram::Skybox, {VS_Sky, PS_Skybox});
			shader_program_map[EShaderProgram::HosekWilkieSky] = std::make_unique<StandardProgram>(device, shader_map[VS_Sky], shader_map[PS_HosekWilkieSky]); AddDependency(EShaderProgram::HosekWilkieSky, {VS_Sky, PS_HosekWilkieSky});
			shader_program_map[EShaderProgram::UniformColorSky] = std::make_unique<StandardProgram>(device, shader_map[VS_Sky], shader_map[PS_UniformColorSky]); AddDependency(EShaderProgram::UniformColorSky, {VS_Sky, PS_UniformColorSky});
			shader_program_map[EShaderProgram::Texture] = std::make_unique<StandardProgram>(device, shader_map[VS_Texture], shader_map[PS_Texture]); AddDependency(EShaderProgram::Texture, {VS_Texture, PS_Texture});
			shader_program_map[EShaderProgram::Solid] = std::make_unique<StandardProgram>(device, shader_map[VS_Solid], shader_map[PS_Solid]); AddDependency(EShaderProgram::Solid, {VS_Solid, PS_Solid});
			shader_program_map[EShaderProgram::Sun] = std::make_unique<StandardProgram>(device, shader_map[VS_Sun], shader_map[PS_Sun]); AddDependency(EShaderProgram::Sun, {VS_Sun, PS_Sun});
			shader_program_map[EShaderProgram::Billboard] = std::make_unique<StandardProgram>(device, shader_map[VS_Billboard], shader_map[PS_Billboard]); AddDependency(EShaderProgram::Billboard, {VS_Billboard, PS_Billboard});
			shader_program_map[EShaderProgram::Decals] = std::make_unique<StandardProgram>(device, shader_map[VS_Decals], shader_map[PS_Decals]); AddDependency(EShaderProgram::Decals, {VS_Decals, PS_Decals});
			shader_program_map[EShaderProgram::Decals_ModifyNormals] = std::make_unique<StandardProgram>(device, shader_map[VS_Decals], shader_map[PS_DecalsModifyNormals]); AddDependency(EShaderProgram::Decals_ModifyNormals, {VS_Decals, PS_DecalsModifyNormals});
			shader_program_map[EShaderProgram::GBuffer_Terrain] = std::make_unique<StandardProgram>(device, shader_map[VS_GBufferTerrain], shader_map[PS_GBufferTerrain]); AddDependency(EShaderProgram::GBuffer_Terrain, {VS_GBufferTerrain, PS_GBufferTerrain});
			shader_program_map[EShaderProgram::GBufferPBR] = std::make_unique<StandardProgram>(device, shader_map[VS_GBufferPBR], shader_map[PS_GBufferPBR]); AddDependency(EShaderProgram::GBufferPBR, {VS_GBufferPBR, PS_GBufferPBR});
			shader_program_map[EShaderProgram::AmbientPBR] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_AmbientPBR]); AddDependency(EShaderProgram::AmbientPBR, {VS_ScreenQuad, PS_AmbientPBR});
			shader_program_map[EShaderProgram::AmbientPBR_AO] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_AmbientPBR_AO]); AddDependency(EShaderProgram::AmbientPBR_AO, {VS_ScreenQuad, PS_AmbientPBR_AO});
			shader_program_map[EShaderProgram::AmbientPBR_IBL] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_AmbientPBR_IBL]); AddDependency(EShaderProgram::AmbientPBR_IBL, {VS_ScreenQuad, PS_AmbientPBR_IBL});
			shader_program_map[EShaderProgram::AmbientPBR_AO_IBL] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_AmbientPBR_AO_IBL]); AddDependency(EShaderProgram::AmbientPBR_AO_IBL, {VS_ScreenQuad, PS_AmbientPBR_AO_IBL});
			shader_program_map[EShaderProgram::LightingPBR] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_LightingPBR]); AddDependency(EShaderProgram::LightingPBR, {VS_ScreenQuad, PS_LightingPBR});
			shader_program_map[EShaderProgram::ClusterLightingPBR] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_ClusterLightingPBR]); AddDependency(EShaderProgram::ClusterLightingPBR, {VS_ScreenQuad, PS_ClusterLightingPBR});
			shader_program_map[EShaderProgram::ToneMap_Reinhard] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_ToneMap_Reinhard]); AddDependency(EShaderProgram::ToneMap_Reinhard, {VS_ScreenQuad, PS_ToneMap_Reinhard});
			shader_program_map[EShaderProgram::ToneMap_Linear] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_ToneMap_Linear]); AddDependency(EShaderProgram::ToneMap_Linear, {VS_ScreenQuad, PS_ToneMap_Linear});
			shader_program_map[EShaderProgram::ToneMap_Hable] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_ToneMap_Hable]); AddDependency(EShaderProgram::ToneMap_Hable, {VS_ScreenQuad, PS_ToneMap_Hable});

			shader_program_map[EShaderProgram::FXAA] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_FXAA]); AddDependency(EShaderProgram::FXAA, {VS_ScreenQuad, PS_FXAA});
			shader_program_map[EShaderProgram::TAA] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_TAA]); AddDependency(EShaderProgram::TAA, {VS_ScreenQuad, PS_TAA});
			shader_program_map[EShaderProgram::Copy] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_Copy]); AddDependency(EShaderProgram::Copy, {VS_ScreenQuad, PS_Copy});
			shader_program_map[EShaderProgram::Add] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_Add]); AddDependency(EShaderProgram::Add, {VS_ScreenQuad, PS_Add});
			shader_program_map[EShaderProgram::SSAO] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_SSAO]); AddDependency(EShaderProgram::SSAO, {VS_ScreenQuad, PS_SSAO});
			shader_program_map[EShaderProgram::HBAO] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_HBAO]); AddDependency(EShaderProgram::HBAO, {VS_ScreenQuad, PS_HBAO});
			shader_program_map[EShaderProgram::SSR] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_SSR]); AddDependency(EShaderProgram::SSR, {VS_ScreenQuad, PS_SSR});
			shader_program_map[EShaderProgram::GodRays] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_GodRays]); AddDependency(EShaderProgram::GodRays, {VS_ScreenQuad, PS_GodRays});
			shader_program_map[EShaderProgram::DOF] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_DepthOfField]); AddDependency(EShaderProgram::DOF, {VS_ScreenQuad, PS_DepthOfField});
			shader_program_map[EShaderProgram::LensFlare] = std::make_unique<GeometryProgram>(device, shader_map[VS_LensFlare], shader_map[GS_LensFlare], shader_map[PS_LensFlare]); AddDependency(EShaderProgram::LensFlare, {VS_LensFlare, GS_LensFlare, PS_LensFlare});
			shader_program_map[EShaderProgram::BokehDraw] = std::make_unique<GeometryProgram>(device, shader_map[VS_Bokeh], shader_map[GS_Bokeh], shader_map[PS_Bokeh]); AddDependency(EShaderProgram::BokehDraw, {VS_Bokeh, GS_Bokeh, PS_Bokeh});

			shader_program_map[EShaderProgram::Volumetric_Clouds] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_VolumetricClouds]); AddDependency(EShaderProgram::Volumetric_Clouds, {VS_ScreenQuad, PS_VolumetricClouds});
			shader_program_map[EShaderProgram::VelocityBuffer] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_VelocityBuffer]); AddDependency(EShaderProgram::VelocityBuffer, {VS_ScreenQuad, PS_VelocityBuffer});
			shader_program_map[EShaderProgram::MotionBlur] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_MotionBlur]); AddDependency(EShaderProgram::MotionBlur, {VS_ScreenQuad, PS_MotionBlur});
			shader_program_map[EShaderProgram::Fog] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_Fog]); AddDependency(EShaderProgram::Fog, {VS_ScreenQuad, PS_Fog});

			shader_program_map[EShaderProgram::DepthMap] = std::make_unique<StandardProgram>(device, shader_map[VS_DepthMap], shader_map[PS_DepthMap]); AddDependency(EShaderProgram::DepthMap, {VS_DepthMap, PS_DepthMap});
			shader_program_map[EShaderProgram::DepthMap_Transparent] = std::make_unique<StandardProgram>(device, shader_map[VS_DepthMapTransparent], shader_map[PS_DepthMapTransparent]); AddDependency(EShaderProgram::DepthMap_Transparent, {VS_DepthMapTransparent, PS_DepthMapTransparent});

			shader_program_map[EShaderProgram::Volumetric_Directional] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_VolumetricLight_Directional]); AddDependency(EShaderProgram::Volumetric_Directional, {VS_ScreenQuad, PS_VolumetricLight_Directional});
			shader_program_map[EShaderProgram::Volumetric_DirectionalCascades] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_VolumetricLight_DirectionalWithCascades]); AddDependency(EShaderProgram::Volumetric_DirectionalCascades, {VS_ScreenQuad, PS_VolumetricLight_DirectionalWithCascades});
			shader_program_map[EShaderProgram::Volumetric_Spot] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_VolumetricLight_Spot]); AddDependency(EShaderProgram::Volumetric_Spot, {VS_ScreenQuad, PS_VolumetricLight_Spot});
			shader_program_map[EShaderProgram::Volumetric_Point] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_VolumetricLight_Point]); AddDependency(EShaderProgram::Volumetric_Point, {VS_ScreenQuad, PS_VolumetricLight_Point});

			shader_program_map[EShaderProgram::Blur_Horizontal] = std::make_unique<ComputeProgram>(device, shader_map[CS_BlurHorizontal]); AddDependency(EShaderProgram::Blur_Horizontal, {CS_BlurHorizontal});
			shader_program_map[EShaderProgram::Blur_Vertical] = std::make_unique<ComputeProgram>(device, shader_map[CS_BlurVertical]); AddDependency(EShaderProgram::Blur_Vertical, {CS_BlurVertical});
			shader_program_map[EShaderProgram::BokehGenerate] = std::make_unique<ComputeProgram>(device, shader_map[CS_BokehGenerate]); AddDependency(EShaderProgram::BokehGenerate, {CS_BokehGenerate});
			shader_program_map[EShaderProgram::BloomExtract] = std::make_unique<ComputeProgram>(device, shader_map[CS_BloomExtract]); AddDependency(EShaderProgram::BloomExtract, {CS_BloomExtract});
			shader_program_map[EShaderProgram::BloomCombine] = std::make_unique<ComputeProgram>(device, shader_map[CS_BloomCombine]); AddDependency(EShaderProgram::BloomCombine, {CS_BloomCombine});

			shader_program_map[EShaderProgram::OceanInitialSpectrum] = std::make_unique<ComputeProgram>(device, shader_map[CS_OceanInitialSpectrum]); AddDependency(EShaderProgram::OceanInitialSpectrum, {CS_OceanInitialSpectrum});
			shader_program_map[EShaderProgram::OceanPhase] = std::make_unique<ComputeProgram>(device, shader_map[CS_OceanPhase]); AddDependency(EShaderProgram::OceanPhase, {CS_OceanPhase});
			shader_program_map[EShaderProgram::OceanSpectrum] = std::make_unique<ComputeProgram>(device, shader_map[CS_OceanSpectrum]); AddDependency(EShaderProgram::OceanSpectrum, {CS_OceanSpectrum});
			shader_program_map[EShaderProgram::OceanNormalMap] = std::make_unique<ComputeProgram>(device, shader_map[CS_OceanNormalMap]); AddDependency(EShaderProgram::OceanNormalMap, {CS_OceanNormalMap});
			shader_program_map[EShaderProgram::OceanFFT_Horizontal] = std::make_unique<ComputeProgram>(device, shader_map[CS_OceanFFT_Horizontal]); AddDependency(EShaderProgram::OceanFFT_Horizontal, {CS_OceanFFT_Horizontal});
			shader_program_map[EShaderProgram::OceanFFT_Vertical] = std::make_unique<ComputeProgram>(device, shader_map[CS_OceanFFT_Vertical]); AddDependency(EShaderProgram::OceanFFT_Vertical, {CS_OceanFFT_Vertical});

			shader_program_map[EShaderProgram::TiledLighting] = std::make_unique<ComputeProgram>(device, shader_map[CS_TiledLighting]); AddDependency(EShaderProgram::TiledLighting, {CS_TiledLighting});
			shader_program_map[EShaderProgram::ClusterBuilding] = std::make_unique<ComputeProgram>(device, shader_map[CS_ClusterBuilding]); AddDependency(EShaderProgram::ClusterBuilding, {CS_ClusterBuilding});
			shader_program_map[EShaderProgram::ClusterCulling] = std::make_unique<ComputeProgram>(device, shader_map[CS_ClusterCulling]); AddDependency(EShaderProgram::ClusterCulling, {CS_ClusterCulling});
			shader_program_map[EShaderProgram::VoxelCopy] = std::make_unique<ComputeProgram>(device, shader_map[CS_VoxelCopy]); AddDependency(EShaderProgram::VoxelCopy, {CS_VoxelCopy});
			shader_program_map[EShaderProgram::VoxelSecondBounce] = std::make_unique<ComputeProgram>(device, shader_map[CS_VoxelSecondBounce]); AddDependency(EShaderProgram::VoxelSecondBounce, {CS_VoxelSecondBounce});

			shader_program_map[EShaderProgram::Ocean] = std::make_unique<StandardProgram>(device, shader_map[VS_Ocean], shader_map[PS_Ocean]); AddDependency(EShaderProgram::Ocean, {VS_Ocean, PS_Ocean});
			shader_program_map[EShaderProgram::OceanLOD] = std::make_unique<TessellationProgram>(device, shader_map[VS_OceanLOD], 
				shader_map[HS_OceanLOD], shader_map[DS_OceanLOD], shader_map[PS_OceanLOD]); AddDependency(EShaderProgram::OceanLOD, { VS_OceanLOD, HS_OceanLOD, DS_OceanLOD, PS_OceanLOD });

			shader_program_map[EShaderProgram::Voxelize] = std::make_unique<GeometryProgram>(device, shader_map[VS_Voxelize], shader_map[GS_Voxelize], shader_map[PS_Voxelize]); AddDependency(EShaderProgram::Voxelize, {VS_Voxelize, GS_Voxelize, PS_Voxelize});
			shader_program_map[EShaderProgram::VoxelizeDebug] = std::make_unique<GeometryProgram>(device, shader_map[VS_VoxelizeDebug], shader_map[GS_VoxelizeDebug], shader_map[PS_VoxelizeDebug]); AddDependency(EShaderProgram::VoxelizeDebug, {VS_VoxelizeDebug, GS_VoxelizeDebug, PS_VoxelizeDebug});
			shader_program_map[EShaderProgram::VoxelGI] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_VoxelGI]); AddDependency(EShaderProgram::VoxelGI, {VS_ScreenQuad, PS_VoxelGI});

			shader_program_map[EShaderProgram::Picker] = std::make_unique<ComputeProgram>(device, shader_map[CS_Picker]); AddDependency(EShaderProgram::Picker, {CS_Picker});

			shader_program_map[EShaderProgram::ParticleInitDeadList] = std::make_unique<ComputeProgram>(device, shader_map[CS_ParticleInitDeadList]); AddDependency(EShaderProgram::ParticleInitDeadList, {CS_ParticleInitDeadList});
			shader_program_map[EShaderProgram::ParticleReset] = std::make_unique<ComputeProgram>(device, shader_map[CS_ParticleReset]); AddDependency(EShaderProgram::ParticleReset, {CS_ParticleReset});
			shader_program_map[EShaderProgram::ParticleEmit] = std::make_unique<ComputeProgram>(device, shader_map[CS_ParticleEmit]); AddDependency(EShaderProgram::ParticleEmit, {CS_ParticleEmit});
			shader_program_map[EShaderProgram::ParticleSimulate] = std::make_unique<ComputeProgram>(device, shader_map[CS_ParticleSimulate]); AddDependency(EShaderProgram::ParticleSimulate, {CS_ParticleSimulate});
			shader_program_map[EShaderProgram::ParticleBitonicSortStep] = std::make_unique<ComputeProgram>(device, shader_map[CS_ParticleBitonicSortStep]); AddDependency(EShaderProgram::ParticleBitonicSortStep, {CS_ParticleBitonicSortStep});
			shader_program_map[EShaderProgram::ParticleSort512] = std::make_unique<ComputeProgram>(device, shader_map[CS_ParticleSort512]); AddDependency(EShaderProgram::ParticleSort512, {CS_ParticleSort512});
			shader_program_map[EShaderProgram::ParticleSortInner512] = std::make_unique<ComputeProgram>(device, shader_map[CS_ParticleSortInner512]); AddDependency(EShaderProgram::ParticleSortInner512, {CS_ParticleSortInner512});
			shader_program_map[EShaderProgram::ParticleSortInitArgs] = std::make_unique<ComputeProgram>(device, shader_map[CS_ParticleSortInitArgs]); AddDependency(EShaderProgram::ParticleSortInitArgs, {CS_ParticleSortInitArgs});
			shader_program_map[EShaderProgram::Particles] = std::make_unique<StandardProgram>(device, shader_map[VS_Particles], shader_map[PS_Particles]); AddDependency(EShaderProgram::Particles, {VS_Particles, PS_Particles});

			std::unique_ptr<StandardProgram> foliage_program = std::make_unique<StandardProgram>(device, shader_map[VS_Foliage], shader_map[PS_Foliage], false);
			std::vector<D3D11_INPUT_ELEMENT_DESC> foliage_input_desc = {
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEX", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "INSTANCE_OFFSET", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
				{ "INSTANCE_ROTATION", 0, DXGI_FORMAT_R32_FLOAT, 1, 12, D3D11_INPUT_PER_INSTANCE_DATA, 1 }
			};
			device->CreateInputLayout(foliage_input_desc.data(), (UINT)foliage_input_desc.size(), shader_map[VS_Foliage].GetPointer(), shader_map[VS_Foliage].GetLength(),
				foliage_program->il.GetAddressOf());
			shader_program_map[EShaderProgram::GBuffer_Foliage] = std::move(foliage_program);
			AddDependency(EShaderProgram::GBuffer_Foliage, { VS_Foliage, PS_Foliage });
		}
		void CompileAllShaders()
		{
			//compiled offline
			{
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/SkyboxVS.cso", shader_map[VS_Sky]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/SkyboxPS.cso", shader_map[PS_Skybox]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/HosekWilkieSkyPS.cso", shader_map[PS_HosekWilkieSky]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/UniformColorSkyPS.cso", shader_map[PS_UniformColorSky]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/TextureVS.cso", shader_map[VS_Texture]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/TexturePS.cso", shader_map[PS_Texture]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/SolidVS.cso", shader_map[VS_Solid]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/SolidPS.cso", shader_map[PS_Solid]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/SunVS.cso", shader_map[VS_Sun]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/SunPS.cso", shader_map[PS_Sun]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/BillboardVS.cso", shader_map[VS_Billboard]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/BillboardPS.cso", shader_map[PS_Billboard]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/DecalVS.cso", shader_map[VS_Decals]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/DecalPS.cso", shader_map[PS_Decals]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/GeometryPassTerrain_VS.cso", shader_map[VS_GBufferTerrain]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/GeometryPassTerrain_PS.cso", shader_map[PS_GBufferTerrain]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/ScreenQuadVS.cso", shader_map[VS_ScreenQuad]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/LightingPBR_PS.cso", shader_map[PS_LightingPBR]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/ClusterLightingPBR_PS.cso", shader_map[PS_ClusterLightingPBR]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/FXAA.cso", shader_map[PS_FXAA]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/TAA_PS.cso", shader_map[PS_TAA]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/CopyPS.cso", shader_map[PS_Copy]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/AddPS.cso", shader_map[PS_Add]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/SSAO_PS.cso", shader_map[PS_SSAO]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/HBAO_PS.cso", shader_map[PS_HBAO]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/SSR_PS.cso", shader_map[PS_SSR]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/LensFlareVS.cso", shader_map[VS_LensFlare]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/LensFlareGS.cso", shader_map[GS_LensFlare]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/LensFlarePS.cso", shader_map[PS_LensFlare]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/GodRaysPS.cso", shader_map[PS_GodRays]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/DOF_PS.cso", shader_map[PS_DepthOfField]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/BokehVS.cso", shader_map[VS_Bokeh]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/BokehGS.cso", shader_map[GS_Bokeh]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/BokehPS.cso", shader_map[PS_Bokeh]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/CloudsPS.cso", shader_map[PS_VolumetricClouds]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/VelocityBufferPS.cso", shader_map[PS_VelocityBuffer]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/MotionBlurPS.cso", shader_map[PS_MotionBlur]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/FogPS.cso", shader_map[PS_Fog]);

				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/VolumetricLightDirectionalPS.cso", shader_map[PS_VolumetricLight_Directional]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/VolumetricLightDirectionalCascadesPS.cso", shader_map[PS_VolumetricLight_DirectionalWithCascades]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/VolumetricLightSpotPS.cso", shader_map[PS_VolumetricLight_Spot]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/VolumetricLightPointPS.cso", shader_map[PS_VolumetricLight_Point]);

				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/BokehCS.cso", shader_map[CS_BokehGenerate]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/BloomExtractCS.cso", shader_map[CS_BloomExtract]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/BloomCombineCS.cso", shader_map[CS_BloomCombine]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/InitialSpectrumCS.cso", shader_map[CS_OceanInitialSpectrum]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/PhaseCS.cso", shader_map[CS_OceanPhase]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/SpectrumCS.cso", shader_map[CS_OceanSpectrum]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/FFT_horizontalCS.cso", shader_map[CS_OceanFFT_Horizontal]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/FFT_verticalCS.cso", shader_map[CS_OceanFFT_Vertical]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/NormalMapCS.cso", shader_map[CS_OceanNormalMap]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/TiledLightingCS.cso", shader_map[CS_TiledLighting]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/ClusterBuildingCS.cso", shader_map[CS_ClusterBuilding]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/ClusterCullingCS.cso", shader_map[CS_ClusterCulling]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/VoxelCopyCS.cso", shader_map[CS_VoxelCopy]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/VoxelSecondBounceCS.cso", shader_map[CS_VoxelSecondBounce]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/OceanVS.cso", shader_map[VS_Ocean]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/OceanPS.cso", shader_map[PS_Ocean]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/OceanLodVS.cso", shader_map[VS_OceanLOD]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/OceanLodPS.cso", shader_map[PS_OceanLOD]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/OceanLodHS.cso", shader_map[HS_OceanLOD]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/OceanLodDS.cso", shader_map[DS_OceanLOD]);

				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/VoxelizeVS.cso", shader_map[VS_Voxelize]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/VoxelizeGS.cso", shader_map[GS_Voxelize]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/VoxelizePS.cso", shader_map[PS_Voxelize]);

				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/VoxelDebugVS.cso", shader_map[VS_VoxelizeDebug]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/VoxelDebugGS.cso", shader_map[GS_VoxelizeDebug]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/VoxelDebugPS.cso", shader_map[PS_VoxelizeDebug]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/VoxelGI_PS.cso", shader_map[PS_VoxelGI]);

				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/FoliageVS.cso", shader_map[VS_Foliage]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/FoliagePS.cso", shader_map[PS_Foliage]);

				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/PickerCS.cso", shader_map[CS_Picker]);

				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/InitDeadListCS.cso", shader_map[CS_ParticleInitDeadList]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/ParticleResetCS.cso", shader_map[CS_ParticleReset]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/ParticleEmitCS.cso", shader_map[CS_ParticleEmit]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/ParticleSimulateCS.cso", shader_map[CS_ParticleSimulate]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/BitonicSortStepCS.cso", shader_map[CS_ParticleBitonicSortStep]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/Sort512CS.cso", shader_map[CS_ParticleSort512]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/SortInner512CS.cso", shader_map[CS_ParticleSortInner512]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/InitSortDispatchArgsCS.cso", shader_map[CS_ParticleSortInitArgs]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/ParticleVS.cso", shader_map[VS_Particles]);
				ShaderCompiler::GetBlobFromCompiledShader("Resources/Compiled Shaders/ParticlePS.cso", shader_map[PS_Particles]);
			}

			ShaderInfo shader_info{ .entrypoint = "main" };
			shader_info.flags =
#if _DEBUG
				ShaderInfo::FLAG_DEBUG | ShaderInfo::FLAG_DISABLE_OPTIMIZATION;
#else
				ShaderInfo::FLAG_NONE;
#endif
			//compiled runtime
			{

				shader_info.shadersource = "Resources/Shaders/Misc/DecalPS.hlsl";
				shader_info.stage = EShaderStage::PS;
				shader_info.macros = { {"DECAL_MODIFY_NORMALS", ""} };

				ShaderCompiler::CompileShader(shader_info, shader_map[PS_DecalsModifyNormals]);

				shader_info.shadersource = "Resources/Shaders/Deferred/GeometryPassPBR_VS.hlsl";
				shader_info.stage = EShaderStage::VS;
				shader_info.macros = {};
				ShaderCompiler::CompileShader(shader_info, shader_map[VS_GBufferPBR]);

				shader_info.shadersource = "Resources/Shaders/Deferred/GeometryPassPBR_PS.hlsl";
				shader_info.stage = EShaderStage::PS;
				ShaderCompiler::CompileShader(shader_info, shader_map[PS_GBufferPBR]);

				shader_info.shadersource = "Resources/Shaders/Deferred/AmbientPBR_PS.hlsl";
				shader_info.stage = EShaderStage::PS;
				ShaderCompiler::CompileShader(shader_info, shader_map[PS_AmbientPBR]);

				shader_info.macros = { {"SSAO", "1"} };
				ShaderCompiler::CompileShader(shader_info, shader_map[PS_AmbientPBR_AO]);

				shader_info.macros = { {"IBL", "1"} };
				ShaderCompiler::CompileShader(shader_info, shader_map[PS_AmbientPBR_IBL]);

				shader_info.macros = { {"SSAO", "1"}, {"IBL", "1"} };
				ShaderCompiler::CompileShader(shader_info, shader_map[PS_AmbientPBR_AO_IBL]);

				shader_info.shadersource = "Resources/Shaders/Postprocess/ToneMapPS.hlsl";
				shader_info.stage = EShaderStage::PS;
				shader_info.macros = { {"REINHARD", "1" } };
				ShaderCompiler::CompileShader(shader_info, shader_map[PS_ToneMap_Reinhard]);
				shader_info.macros = { {"LINEAR", "1" } };
				ShaderCompiler::CompileShader(shader_info, shader_map[PS_ToneMap_Linear]);
				shader_info.macros = { {"HABLE", "1" } };
				ShaderCompiler::CompileShader(shader_info, shader_map[PS_ToneMap_Hable]);

				shader_info.shadersource = "Resources/Shaders/Shadows/DepthMapVS.hlsl";
				shader_info.stage = EShaderStage::VS;
				shader_info.macros = {};
				ShaderCompiler::CompileShader(shader_info, shader_map[VS_DepthMap]);
				shader_info.macros = { {"TRANSPARENT", "1"} };
				ShaderCompiler::CompileShader(shader_info, shader_map[VS_DepthMapTransparent]);

				shader_info.shadersource = "Resources/Shaders/Shadows/DepthMapPS.hlsl";
				shader_info.stage = EShaderStage::PS;
				shader_info.macros = {};
				ShaderCompiler::CompileShader(shader_info, shader_map[PS_DepthMap]);
				shader_info.macros = { {"TRANSPARENT", "1"} };
				ShaderCompiler::CompileShader(shader_info, shader_map[PS_DepthMapTransparent]);

				shader_info.shadersource = "Resources/Shaders/Postprocess/BlurCS.hlsl";
				shader_info.stage = EShaderStage::CS;
				shader_info.macros = {};
				ShaderCompiler::CompileShader(shader_info, shader_map[CS_BlurHorizontal]);
				shader_info.macros = { { "VERTICAL", "1" } };
				ShaderCompiler::CompileShader(shader_info, shader_map[CS_BlurVertical]);
			}

			CreateAllPrograms();
		}
		void FillShaderFileDataMap()
		{
			using UnderlyingType = std::underlying_type_t<EShader>;
			for (UnderlyingType i = 0; i < EShader_Count; ++i)
			{
				ShaderFileData file_data{};
				file_data.last_changed_timestamp = std::filesystem::last_write_time(fs::path(shaders_directory + GetShaderSource((EShader)i)));
				shader_file_data_map[(EShader)i] = file_data;
			}
		}

		void RecreateDependentPrograms(EShader shader)
		{
			if (shader == VS_Foliage || shader == PS_Foliage)
			{
				std::unique_ptr<StandardProgram> foliage_program = std::make_unique<StandardProgram>(device, shader_map[VS_Foliage], shader_map[PS_Foliage], false);
				std::vector<D3D11_INPUT_ELEMENT_DESC> foliage_input_desc = {
					{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
					{ "TEX", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
					{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
					{ "INSTANCE_OFFSET", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
					{ "INSTANCE_ROTATION", 0, DXGI_FORMAT_R32_FLOAT, 1, 12, D3D11_INPUT_PER_INSTANCE_DATA, 1 }
				};
				device->CreateInputLayout(foliage_input_desc.data(), (UINT)foliage_input_desc.size(), shader_map[VS_Foliage].GetPointer(), shader_map[VS_Foliage].GetLength(),
					foliage_program->il.GetAddressOf());
				shader_program_map[EShaderProgram::GBuffer_Foliage] = std::move(foliage_program);
				return;
			}
			for (auto const& [program, shaders] : dependency_map)
			{
				if (auto it = std::find(std::begin(shaders), std::end(shaders), shader); it != std::end(shaders))
				{
					switch (shaders.size())
					{
					case 1:
						shader_program_map[program] = std::make_unique<ComputeProgram>(device, shader_map[shaders[0]]);
						break;
					case 2:
						shader_program_map[program] = std::make_unique<StandardProgram>(device, shader_map[shaders[0]], shader_map[shaders[1]]);
						break;
					case 3:
						shader_program_map[program] = std::make_unique<GeometryProgram>(device, shader_map[shaders[0]], shader_map[shaders[1]], shader_map[shaders[2]]);
						break;
					case 4:
						shader_program_map[program] = std::make_unique<TessellationProgram>(device, shader_map[shaders[0]], shader_map[shaders[1]], shader_map[shaders[2]], shader_map[shaders[3]]);
						break;
					}
				}
			}
		}
		bool HasShaderChanged(EShader shader)
		{
			std::string shader_source = GetShaderSource(shader);
			fs::file_time_type curr_timestamp = std::filesystem::last_write_time(fs::path(shaders_directory + GetShaderSource(shader)));
			bool has_changed = shader_file_data_map[shader].last_changed_timestamp != curr_timestamp;
			if (has_changed) shader_file_data_map[shader].last_changed_timestamp = curr_timestamp;
			return has_changed;
		}
	}

	void ShaderCache::Initialize(ID3D11Device* _device)
	{
		device = _device;
		CompileAllShaders();
		FillShaderFileDataMap();
	}

	void ShaderCache::Destroy()
	{
		device = nullptr;
		FreeContainer(shader_map);
		FreeContainer(shader_file_data_map);
		FreeContainer(shader_program_map);
		FreeContainer(dependency_map);
	}

	void ShaderCache::RecompileShader(EShader shader, bool recreate_programs)
	{
		ShaderInfo shader_info{ .entrypoint = "main" };
		shader_info.flags =
#if _DEBUG
			ShaderInfo::FLAG_DEBUG | ShaderInfo::FLAG_DISABLE_OPTIMIZATION;
#else
			ShaderInfo::FLAG_NONE;
#endif
		shader_info.shadersource = std::string(shaders_directory) + GetShaderSource(shader);
		shader_info.stage = GetStage(shader);
		shader_info.macros = GetShaderMacros(shader);

		ShaderCompiler::CompileShader(shader_info, shader_map[shader]);
		if(recreate_programs) RecreateDependentPrograms(shader);
	}

	ShaderProgram* ShaderCache::GetShaderProgram(EShaderProgram shader_program)
	{
		return shader_program_map[shader_program].get();
	}

	void ShaderCache::RecompileChangedShaders()
	{
		ADRIA_LOG(INFO, "Recompiling changed shaders...");
		using UnderlyingType = std::underlying_type_t<EShader>;
		static EngineTimer timer;

		timer.Mark();
		std::vector<UnderlyingType> shaders(EShader_Count);
		std::iota(std::begin(shaders), std::end(shaders), 0);
		std::for_each(
			std::execution::par_unseq,
			std::begin(shaders),
			std::end(shaders),
			[](UnderlyingType s)
			{
				if (HasShaderChanged((EShader)s))
				{
					RecompileShader((EShader)s, false);
					RecreateDependentPrograms((EShader)s);
				}
			});
		ADRIA_LOG(INFO, "Compilation done in %f s!", timer.MarkInSeconds());
	}

	void ShaderCache::RecompileAllShaders()
	{
		ADRIA_LOG(INFO, "Recompiling all shaders...");
		using UnderlyingType = std::underlying_type_t<EShader>;

		std::vector<UnderlyingType> shaders(EShader_Count);
		std::iota(std::begin(shaders), std::end(shaders), 0);
		std::for_each(
			std::execution::par_unseq,
			std::begin(shaders),
			std::end(shaders),
			[](UnderlyingType s)
			{
				RecompileShader((EShader)s, false);
			});
		CreateAllPrograms();
		ADRIA_LOG(INFO, "Compilation done!");
	}

}

