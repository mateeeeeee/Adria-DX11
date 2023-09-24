#include <unordered_map>
#include <memory>
#include <string_view>
#include <execution>
#include <filesystem>
#include "ShaderManager.h"
#include "Graphics/GfxShaderProgram.h"
#include "Graphics/GfxShaderCompiler.h"
#include "Logging/Logger.h"
#include "Utilities/Timer.h"
#include "Utilities/HashMap.h"
#include "Utilities/HashSet.h"
#include "Utilities/FileWatcher.h"

namespace fs = std::filesystem;

namespace adria
{
	namespace
	{
		ID3D11Device* device;
		std::unique_ptr<FileWatcher> file_watcher;
		DelegateHandle file_modified_handle;

		HashMap<ShaderId, std::unique_ptr<GfxVertexShader>>		vs_shader_map;
		HashMap<ShaderId, std::unique_ptr<GfxPixelShader>>		ps_shader_map;
		HashMap<ShaderId, std::unique_ptr<GfxHullShader>>		hs_shader_map;
		HashMap<ShaderId, std::unique_ptr<GfxDomainShader>>		ds_shader_map;
		HashMap<ShaderId, std::unique_ptr<GfxGeometryShader>>	gs_shader_map;
		HashMap<ShaderId, std::unique_ptr<GfxComputeShader>>	cs_shader_map;
		HashMap<ShaderId, HashSet<fs::path>>					dependent_files_map;
		HashMap<ShaderId, GfxInputLayout>						input_layout_map;

		HashMap<ShaderProgram, GfxGraphicsShaderProgram>		gfx_shader_program_map;
		HashMap<ShaderProgram, GfxComputeShaderProgram> compute_shader_program_map;

		constexpr GfxShaderStage GetStage(ShaderId shader)
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
				return GfxShaderStage::VS;
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
			case PS_GBufferPBR_Mask:
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
				return GfxShaderStage::PS;
			case GS_LensFlare:
			case GS_Bokeh:
			case GS_Voxelize:
			case GS_VoxelizeDebug:
				return GfxShaderStage::GS;
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
				return GfxShaderStage::CS;
			case HS_OceanLOD:
				return GfxShaderStage::HS;
			case DS_OceanLOD:
				return GfxShaderStage::DS;
			case EShader_Count:
			default:
				return GfxShaderStage::StageCount;
			}
		}
		constexpr std::string GetShaderSource(ShaderId shader)
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
				return "Deferred/GBuffer_VS.hlsl";
			case PS_GBufferPBR:
			case PS_GBufferPBR_Mask:
				return "Deferred/GBuffer_PS.hlsl";
			case VS_GBufferTerrain:
				return "Deferred/TerrainGBuffer_VS.hlsl";
			case PS_GBufferTerrain:
				return "Deferred/TerrainGBuffer_PS.hlsl";
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
		constexpr std::vector<GfxShaderMacro> GetShaderMacros(ShaderId shader)
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
			case PS_GBufferPBR_Mask:
				return { { "MASK", "1" } };
			default:
				return {};
			}
		}

		void CompileShader(ShaderId shader, bool first_compile = false)
		{
			GfxShaderDesc input{ .entrypoint = "main" };
#if _DEBUG
			input.flags = GfxShaderCompilerFlagBit_Debug | GfxShaderCompilerFlagBit_DisableOptimization;
#else
			input.flags = GfxShaderCompilerFlagBit_None;
#endif
			input.source_file = "Resources/Shaders/" + GetShaderSource(shader);
			input.stage = GetStage(shader);
			input.macros = GetShaderMacros(shader);

			GfxShaderCompileOutput output{};
			GfxShaderCompiler::CompileShader(input, output);
			switch (input.stage)
			{
			case GfxShaderStage::VS:
				if(first_compile) vs_shader_map[shader] = std::make_unique<GfxVertexShader>(device, output.shader_bytecode);
				else vs_shader_map[shader]->Recreate(device, output.shader_bytecode);
				break;
			case GfxShaderStage::PS:
				if (first_compile) ps_shader_map[shader] = std::make_unique<GfxPixelShader>(device, output.shader_bytecode);
				else ps_shader_map[shader]->Recreate(device, output.shader_bytecode);
				break;
			case GfxShaderStage::HS:
				if (first_compile) hs_shader_map[shader] = std::make_unique<GfxHullShader>(device, output.shader_bytecode);
				else hs_shader_map[shader]->Recreate(device, output.shader_bytecode);
				break;
			case GfxShaderStage::DS:
				if (first_compile) ds_shader_map[shader] = std::make_unique<GfxDomainShader>(device, output.shader_bytecode);
				else ds_shader_map[shader]->Recreate(device, output.shader_bytecode);
				break;
			case GfxShaderStage::GS:
				if (first_compile) gs_shader_map[shader] = std::make_unique<GfxGeometryShader>(device, output.shader_bytecode);
				else gs_shader_map[shader]->Recreate(device, output.shader_bytecode);
				break;
			case GfxShaderStage::CS:
				if (first_compile) cs_shader_map[shader] = std::make_unique<GfxComputeShader>(device, output.shader_bytecode);
				else cs_shader_map[shader]->Recreate(device, output.shader_bytecode);
				break;
			default:
				ADRIA_ASSERT(false);
			}
			dependent_files_map[shader].clear();
			dependent_files_map[shader].insert(output.includes.begin(), output.includes.end());
		}
		void CreateAllPrograms()
		{
			using UnderlyingType = std::underlying_type_t<ShaderId>;
			for (UnderlyingType s = 0; s < EShader_Count; ++s)
			{
				ShaderId shader = (ShaderId)s;
				if (GetStage(shader) != GfxShaderStage::VS) continue;
				
				if (shader != VS_Foliage) input_layout_map.emplace(std::piecewise_construct, 
																   std::forward_as_tuple(shader),
																   std::forward_as_tuple(device, vs_shader_map[shader]->GetBytecode()));
				else
				{
					std::vector<D3D11_INPUT_ELEMENT_DESC> foliage_input_desc = {
						{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
						{ "TEX", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
						{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
						{ "INSTANCE_OFFSET", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1 },
						{ "INSTANCE_ROTATION", 0, DXGI_FORMAT_R32_FLOAT, 1, 12, D3D11_INPUT_PER_INSTANCE_DATA, 1 }
					};
					input_layout_map.emplace(std::piecewise_construct,
						std::forward_as_tuple(shader),
						std::forward_as_tuple(device, vs_shader_map[shader]->GetBytecode(), foliage_input_desc));
				}
			}

			gfx_shader_program_map[ShaderProgram::Skybox].SetVertexShader(vs_shader_map[VS_Sky].get()).SetPixelShader(ps_shader_map[PS_Skybox].get()).SetInputLayout(&input_layout_map[VS_Sky]); 
			gfx_shader_program_map[ShaderProgram::HosekWilkieSky].SetVertexShader(vs_shader_map[VS_Sky].get()).SetPixelShader(ps_shader_map[PS_HosekWilkieSky].get()).SetInputLayout(&input_layout_map[VS_Sky]); 
			gfx_shader_program_map[ShaderProgram::UniformColorSky].SetVertexShader(vs_shader_map[VS_Sky].get()).SetPixelShader(ps_shader_map[PS_UniformColorSky].get()).SetInputLayout(&input_layout_map[VS_Sky]); 
			gfx_shader_program_map[ShaderProgram::Texture].SetVertexShader(vs_shader_map[VS_Texture].get()).SetPixelShader(ps_shader_map[PS_Texture].get()).SetInputLayout(&input_layout_map[VS_Texture]); 
			gfx_shader_program_map[ShaderProgram::Solid].SetVertexShader(vs_shader_map[VS_Solid].get()).SetPixelShader(ps_shader_map[PS_Solid].get()).SetInputLayout(&input_layout_map[VS_Solid]); 
			gfx_shader_program_map[ShaderProgram::Sun].SetVertexShader(vs_shader_map[VS_Sun].get()).SetPixelShader(ps_shader_map[PS_Sun].get()).SetInputLayout(&input_layout_map[VS_Sun]); 
			gfx_shader_program_map[ShaderProgram::Billboard].SetVertexShader(vs_shader_map[VS_Billboard].get()).SetPixelShader(ps_shader_map[PS_Billboard].get()).SetInputLayout(&input_layout_map[VS_Billboard]); 
			gfx_shader_program_map[ShaderProgram::Decals].SetVertexShader(vs_shader_map[VS_Decals].get()).SetPixelShader(ps_shader_map[PS_Decals].get()).SetInputLayout(&input_layout_map[VS_Decals]); 
			gfx_shader_program_map[ShaderProgram::Decals_ModifyNormals].SetVertexShader(vs_shader_map[VS_Decals].get()).SetPixelShader(ps_shader_map[PS_DecalsModifyNormals].get()).SetInputLayout(&input_layout_map[VS_Decals]); 
			gfx_shader_program_map[ShaderProgram::GBuffer_Terrain].SetVertexShader(vs_shader_map[VS_GBufferTerrain].get()).SetPixelShader(ps_shader_map[PS_GBufferTerrain].get()).SetInputLayout(&input_layout_map[VS_GBufferTerrain]); 
			gfx_shader_program_map[ShaderProgram::GBufferPBR].SetVertexShader(vs_shader_map[VS_GBufferPBR].get()).SetPixelShader(ps_shader_map[PS_GBufferPBR].get()).SetInputLayout(&input_layout_map[VS_GBufferPBR]); 
			gfx_shader_program_map[ShaderProgram::GBufferPBR_Mask].SetVertexShader(vs_shader_map[VS_GBufferPBR].get()).SetPixelShader(ps_shader_map[PS_GBufferPBR_Mask].get()).SetInputLayout(&input_layout_map[VS_GBufferPBR]); 
			gfx_shader_program_map[ShaderProgram::AmbientPBR].SetVertexShader(vs_shader_map[VS_ScreenQuad].get()).SetPixelShader(ps_shader_map[PS_AmbientPBR].get()).SetInputLayout(&input_layout_map[VS_ScreenQuad]); 
			gfx_shader_program_map[ShaderProgram::AmbientPBR_AO].SetVertexShader(vs_shader_map[VS_ScreenQuad].get()).SetPixelShader(ps_shader_map[PS_AmbientPBR_AO].get()).SetInputLayout(&input_layout_map[VS_ScreenQuad]); 
			gfx_shader_program_map[ShaderProgram::AmbientPBR_IBL].SetVertexShader(vs_shader_map[VS_ScreenQuad].get()).SetPixelShader(ps_shader_map[PS_AmbientPBR_IBL].get()).SetInputLayout(&input_layout_map[VS_ScreenQuad]); 
			gfx_shader_program_map[ShaderProgram::AmbientPBR_AO_IBL].SetVertexShader(vs_shader_map[VS_ScreenQuad].get()).SetPixelShader(ps_shader_map[PS_AmbientPBR_AO_IBL].get()).SetInputLayout(&input_layout_map[VS_ScreenQuad]); 
			gfx_shader_program_map[ShaderProgram::LightingPBR].SetVertexShader(vs_shader_map[VS_ScreenQuad].get()).SetPixelShader(ps_shader_map[PS_LightingPBR].get()).SetInputLayout(&input_layout_map[VS_ScreenQuad]); 
			gfx_shader_program_map[ShaderProgram::ClusterLightingPBR].SetVertexShader(vs_shader_map[VS_ScreenQuad].get()).SetPixelShader(ps_shader_map[PS_ClusterLightingPBR].get()).SetInputLayout(&input_layout_map[VS_ScreenQuad]); 
			gfx_shader_program_map[ShaderProgram::ToneMap_Reinhard].SetVertexShader(vs_shader_map[VS_ScreenQuad].get()).SetPixelShader(ps_shader_map[PS_ToneMap_Reinhard].get()).SetInputLayout(&input_layout_map[VS_ScreenQuad]); 
			gfx_shader_program_map[ShaderProgram::ToneMap_Linear].SetVertexShader(vs_shader_map[VS_ScreenQuad].get()).SetPixelShader(ps_shader_map[PS_ToneMap_Linear].get()).SetInputLayout(&input_layout_map[VS_ScreenQuad]); 
			gfx_shader_program_map[ShaderProgram::ToneMap_Hable].SetVertexShader(vs_shader_map[VS_ScreenQuad].get()).SetPixelShader(ps_shader_map[PS_ToneMap_Hable].get()).SetInputLayout(&input_layout_map[VS_ScreenQuad]); 

			gfx_shader_program_map[ShaderProgram::FXAA].SetVertexShader(vs_shader_map[VS_ScreenQuad].get()).SetPixelShader(ps_shader_map[PS_FXAA].get()).SetInputLayout(&input_layout_map[VS_ScreenQuad]); 
			gfx_shader_program_map[ShaderProgram::TAA].SetVertexShader(vs_shader_map[VS_ScreenQuad].get()).SetPixelShader(ps_shader_map[PS_TAA].get()).SetInputLayout(&input_layout_map[VS_ScreenQuad]); 
			gfx_shader_program_map[ShaderProgram::Copy].SetVertexShader(vs_shader_map[VS_ScreenQuad].get()).SetPixelShader(ps_shader_map[PS_Copy].get()).SetInputLayout(&input_layout_map[VS_ScreenQuad]); 
			gfx_shader_program_map[ShaderProgram::Add].SetVertexShader(vs_shader_map[VS_ScreenQuad].get()).SetPixelShader(ps_shader_map[PS_Add].get()).SetInputLayout(&input_layout_map[VS_ScreenQuad]); 
			gfx_shader_program_map[ShaderProgram::SSAO].SetVertexShader(vs_shader_map[VS_ScreenQuad].get()).SetPixelShader(ps_shader_map[PS_SSAO].get()).SetInputLayout(&input_layout_map[VS_ScreenQuad]); 
			gfx_shader_program_map[ShaderProgram::HBAO].SetVertexShader(vs_shader_map[VS_ScreenQuad].get()).SetPixelShader(ps_shader_map[PS_HBAO].get()).SetInputLayout(&input_layout_map[VS_ScreenQuad]); 
			gfx_shader_program_map[ShaderProgram::SSR].SetVertexShader(vs_shader_map[VS_ScreenQuad].get()).SetPixelShader(ps_shader_map[PS_SSR].get()).SetInputLayout(&input_layout_map[VS_ScreenQuad]); 
			gfx_shader_program_map[ShaderProgram::GodRays].SetVertexShader(vs_shader_map[VS_ScreenQuad].get()).SetPixelShader(ps_shader_map[PS_GodRays].get()).SetInputLayout(&input_layout_map[VS_ScreenQuad]); 
			gfx_shader_program_map[ShaderProgram::DOF].SetVertexShader(vs_shader_map[VS_ScreenQuad].get()).SetPixelShader(ps_shader_map[PS_DepthOfField].get()).SetInputLayout(&input_layout_map[VS_ScreenQuad]); 
			gfx_shader_program_map[ShaderProgram::LensFlare].SetVertexShader(vs_shader_map[VS_LensFlare].get()).SetGeometryShader(gs_shader_map[GS_LensFlare].get()).SetPixelShader(ps_shader_map[PS_LensFlare].get()).SetInputLayout(&input_layout_map[VS_LensFlare]); 
			gfx_shader_program_map[ShaderProgram::BokehDraw].SetVertexShader(vs_shader_map[VS_Bokeh].get()).SetGeometryShader(gs_shader_map[GS_Bokeh].get()).SetPixelShader(ps_shader_map[PS_Bokeh].get()).SetInputLayout(&input_layout_map[VS_Bokeh]); 

			gfx_shader_program_map[ShaderProgram::Volumetric_Clouds].SetVertexShader(vs_shader_map[VS_ScreenQuad].get()).SetPixelShader(ps_shader_map[PS_VolumetricClouds].get()).SetInputLayout(&input_layout_map[VS_ScreenQuad]); 
			gfx_shader_program_map[ShaderProgram::VelocityBuffer].SetVertexShader(vs_shader_map[VS_ScreenQuad].get()).SetPixelShader(ps_shader_map[PS_VelocityBuffer].get()).SetInputLayout(&input_layout_map[VS_ScreenQuad]); 
			gfx_shader_program_map[ShaderProgram::MotionBlur].SetVertexShader(vs_shader_map[VS_ScreenQuad].get()).SetPixelShader(ps_shader_map[PS_MotionBlur].get()).SetInputLayout(&input_layout_map[VS_ScreenQuad]); 
			gfx_shader_program_map[ShaderProgram::Fog].SetVertexShader(vs_shader_map[VS_ScreenQuad].get()).SetPixelShader(ps_shader_map[PS_Fog].get()).SetInputLayout(&input_layout_map[VS_ScreenQuad]); 

			gfx_shader_program_map[ShaderProgram::DepthMap].SetVertexShader(vs_shader_map[VS_DepthMap].get()).SetPixelShader(ps_shader_map[PS_DepthMap].get()).SetInputLayout(&input_layout_map[VS_DepthMap]); 
			gfx_shader_program_map[ShaderProgram::DepthMap_Transparent].SetVertexShader(vs_shader_map[VS_DepthMapTransparent].get()).SetPixelShader(ps_shader_map[PS_DepthMapTransparent].get()).SetInputLayout(&input_layout_map[VS_DepthMapTransparent]); 

			gfx_shader_program_map[ShaderProgram::Volumetric_Directional].SetVertexShader(vs_shader_map[VS_ScreenQuad].get()).SetPixelShader(ps_shader_map[PS_VolumetricLight_Directional].get()).SetInputLayout(&input_layout_map[VS_ScreenQuad]); 
			gfx_shader_program_map[ShaderProgram::Volumetric_DirectionalCascades].SetVertexShader(vs_shader_map[VS_ScreenQuad].get()).SetPixelShader(ps_shader_map[PS_VolumetricLight_DirectionalWithCascades].get()).SetInputLayout(&input_layout_map[VS_ScreenQuad]); 
			gfx_shader_program_map[ShaderProgram::Volumetric_Spot].SetVertexShader(vs_shader_map[VS_ScreenQuad].get()).SetPixelShader(ps_shader_map[PS_VolumetricLight_Spot].get()).SetInputLayout(&input_layout_map[VS_ScreenQuad]); 
			gfx_shader_program_map[ShaderProgram::Volumetric_Point].SetVertexShader(vs_shader_map[VS_ScreenQuad].get()).SetPixelShader(ps_shader_map[PS_VolumetricLight_Point].get()).SetInputLayout(&input_layout_map[VS_ScreenQuad]); 

			compute_shader_program_map[ShaderProgram::Blur_Horizontal].SetComputeShader(cs_shader_map[CS_BlurHorizontal].get()); 
			compute_shader_program_map[ShaderProgram::Blur_Vertical].SetComputeShader(cs_shader_map[CS_BlurVertical].get()); 
			compute_shader_program_map[ShaderProgram::BokehGenerate].SetComputeShader(cs_shader_map[CS_BokehGenerate].get()); 
			compute_shader_program_map[ShaderProgram::BloomExtract].SetComputeShader(cs_shader_map[CS_BloomExtract].get()); 
			compute_shader_program_map[ShaderProgram::BloomCombine].SetComputeShader(cs_shader_map[CS_BloomCombine].get()); 

			compute_shader_program_map[ShaderProgram::OceanInitialSpectrum].SetComputeShader(cs_shader_map[CS_OceanInitialSpectrum].get()); 
			compute_shader_program_map[ShaderProgram::OceanPhase].SetComputeShader(cs_shader_map[CS_OceanPhase].get()); 
			compute_shader_program_map[ShaderProgram::OceanSpectrum].SetComputeShader(cs_shader_map[CS_OceanSpectrum].get()); 
			compute_shader_program_map[ShaderProgram::OceanNormalMap].SetComputeShader(cs_shader_map[CS_OceanNormalMap].get()); 
			compute_shader_program_map[ShaderProgram::OceanFFT_Horizontal].SetComputeShader(cs_shader_map[CS_OceanFFT_Horizontal].get()); 
			compute_shader_program_map[ShaderProgram::OceanFFT_Vertical].SetComputeShader(cs_shader_map[CS_OceanFFT_Vertical].get()); 

			compute_shader_program_map[ShaderProgram::TiledLighting].SetComputeShader(cs_shader_map[CS_TiledLighting].get()); 
			compute_shader_program_map[ShaderProgram::ClusterBuilding].SetComputeShader(cs_shader_map[CS_ClusterBuilding].get()); 
			compute_shader_program_map[ShaderProgram::ClusterCulling].SetComputeShader(cs_shader_map[CS_ClusterCulling].get()); 
			compute_shader_program_map[ShaderProgram::VoxelCopy].SetComputeShader(cs_shader_map[CS_VoxelCopy].get()); 
			compute_shader_program_map[ShaderProgram::VoxelSecondBounce].SetComputeShader(cs_shader_map[CS_VoxelSecondBounce].get()); 

			gfx_shader_program_map[ShaderProgram::Ocean].SetVertexShader(vs_shader_map[VS_Ocean].get()).SetPixelShader(ps_shader_map[PS_Ocean].get()).SetInputLayout(&input_layout_map[VS_Ocean]); 
			gfx_shader_program_map[ShaderProgram::OceanLOD].SetVertexShader(vs_shader_map[VS_OceanLOD].get()).SetHullShader(hs_shader_map[HS_OceanLOD].get()).SetDomainShader(ds_shader_map[DS_OceanLOD].get()).
				SetPixelShader(ps_shader_map[PS_OceanLOD].get()).SetInputLayout(&input_layout_map[VS_OceanLOD]);
				
			gfx_shader_program_map[ShaderProgram::Voxelize].SetVertexShader(vs_shader_map[VS_Voxelize].get()).SetGeometryShader(gs_shader_map[GS_Voxelize].get()).SetPixelShader(ps_shader_map[PS_Voxelize].get()).SetInputLayout(&input_layout_map[VS_Voxelize]); 
			gfx_shader_program_map[ShaderProgram::VoxelizeDebug].SetVertexShader(vs_shader_map[VS_VoxelizeDebug].get()).SetGeometryShader(gs_shader_map[GS_VoxelizeDebug].get()).SetPixelShader(ps_shader_map[PS_VoxelizeDebug].get()).SetInputLayout(&input_layout_map[VS_VoxelizeDebug]); 
			gfx_shader_program_map[ShaderProgram::VoxelGI].SetVertexShader(vs_shader_map[VS_ScreenQuad].get()).SetPixelShader(ps_shader_map[PS_VoxelGI].get()).SetInputLayout(&input_layout_map[VS_ScreenQuad]); 

			compute_shader_program_map[ShaderProgram::Picker].SetComputeShader(cs_shader_map[CS_Picker].get()); 

			compute_shader_program_map[ShaderProgram::ParticleInitDeadList].SetComputeShader(cs_shader_map[CS_ParticleInitDeadList].get()); 
			compute_shader_program_map[ShaderProgram::ParticleReset].SetComputeShader(cs_shader_map[CS_ParticleReset].get()); 
			compute_shader_program_map[ShaderProgram::ParticleEmit].SetComputeShader(cs_shader_map[CS_ParticleEmit].get()); 
			compute_shader_program_map[ShaderProgram::ParticleSimulate].SetComputeShader(cs_shader_map[CS_ParticleSimulate].get()); 
			compute_shader_program_map[ShaderProgram::ParticleBitonicSortStep].SetComputeShader(cs_shader_map[CS_ParticleBitonicSortStep].get()); 
			compute_shader_program_map[ShaderProgram::ParticleSort512].SetComputeShader(cs_shader_map[CS_ParticleSort512].get()); 
			compute_shader_program_map[ShaderProgram::ParticleSortInner512].SetComputeShader(cs_shader_map[CS_ParticleSortInner512].get()); 
			compute_shader_program_map[ShaderProgram::ParticleSortInitArgs].SetComputeShader(cs_shader_map[CS_ParticleSortInitArgs].get()); 
			gfx_shader_program_map[ShaderProgram::Particles].SetVertexShader(vs_shader_map[VS_Particles].get()).SetPixelShader(ps_shader_map[PS_Particles].get()).SetInputLayout(&input_layout_map[VS_Particles]); 
			gfx_shader_program_map[ShaderProgram::GBuffer_Foliage].SetVertexShader(vs_shader_map[VS_Foliage].get()).SetPixelShader(ps_shader_map[PS_Foliage].get()).SetInputLayout(&input_layout_map[VS_Foliage]);
		}
		void CompileAllShaders()
		{
			Timer t;
			ADRIA_LOG(INFO, "Compiling all shaders...");
			using UnderlyingType = std::underlying_type_t<ShaderId>;

			std::vector<UnderlyingType> shaders(EShader_Count);
			std::iota(std::begin(shaders), std::end(shaders), 0);
			std::for_each(
				std::execution::seq,
				std::begin(shaders),
				std::end(shaders),
				[](UnderlyingType s)
				{
					CompileShader((ShaderId)s, true);
				});
			CreateAllPrograms();
			ADRIA_LOG(INFO, "Compilation done in %f seconds!", t.ElapsedInSeconds());
		}
		void OnShaderFileChanged(std::string const& filename)
		{
			for (auto const& [shader, files] : dependent_files_map)
			{
				if (files.contains(fs::path(filename))) CompileShader(shader);
			}
		}
	}

	void ShaderManager::Initialize(ID3D11Device* _device)
	{
		device = _device;
		file_watcher = std::make_unique<FileWatcher>();
		CompileAllShaders();
		file_watcher->AddPathToWatch("Resources/Shaders/");
		std::ignore = file_watcher->GetFileModifiedEvent().Add(OnShaderFileChanged);
	}

	void ShaderManager::Destroy()
	{
		device = nullptr;
		file_watcher = nullptr;
		auto FreeContainer = []<typename T>(T& container) 
		{
			container.clear();
			T empty;
			using std::swap;
			swap(container, empty);
		};
		FreeContainer(gfx_shader_program_map);
		FreeContainer(compute_shader_program_map);
		FreeContainer(vs_shader_map);
		FreeContainer(ps_shader_map);
		FreeContainer(hs_shader_map);
		FreeContainer(ds_shader_map);
		FreeContainer(gs_shader_map);
		FreeContainer(cs_shader_map);
		FreeContainer(input_layout_map);
	}

	GfxShaderProgram* ShaderManager::GetShaderProgram(ShaderProgram shader_program)
	{
		bool is_gfx_program = gfx_shader_program_map.contains(shader_program);
		if (is_gfx_program) return &gfx_shader_program_map[shader_program];
		else return &compute_shader_program_map[shader_program];
	}

	void ShaderManager::CheckIfShadersHaveChanged()
	{
		file_watcher->CheckWatchedFiles();
	}
}

