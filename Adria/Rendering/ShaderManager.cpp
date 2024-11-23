#include <unordered_map>
#include <set>
#include <memory>
#include <string_view>
#include <execution>
#include <filesystem>
#include "ShaderManager.h"
#include "Core/Logger.h"
#include "Core/Paths.h"
#include "Graphics/GfxShaderProgram.h"
#include "Graphics/GfxShaderCompiler.h"
#include "Graphics/GfxDevice.h"
#include "Graphics/GfxInputLayout.h"
#include "Utilities/Timer.h"
#include "Utilities/HashSet.h"
#include "Utilities/FileWatcher.h"

namespace fs = std::filesystem;

namespace adria
{
	namespace
	{
		GfxDevice* device;
		std::unique_ptr<FileWatcher> file_watcher;
		DelegateHandle file_modified_handle;

		std::unordered_map<ShaderId, std::unique_ptr<GfxVertexShader>>		vs_shader_map;
		std::unordered_map<ShaderId, std::unique_ptr<GfxPixelShader>>		ps_shader_map;
		std::unordered_map<ShaderId, std::unique_ptr<GfxHullShader>>		hs_shader_map;
		std::unordered_map<ShaderId, std::unique_ptr<GfxDomainShader>>		ds_shader_map;
		std::unordered_map<ShaderId, std::unique_ptr<GfxGeometryShader>>	gs_shader_map;
		std::unordered_map<ShaderId, std::unique_ptr<GfxComputeShader>>		cs_shader_map;
		std::unordered_map<ShaderId, std::unique_ptr<GfxInputLayout>>		input_layout_map;
		std::unordered_map<fs::path, std::set<ShaderId>>					file_shader_map;

		std::unordered_map<ShaderProgram, GfxGraphicsShaderProgram>			gfx_shader_program_map;
		std::unordered_map<ShaderProgram, GfxComputeShaderProgram>			compute_shader_program_map;

		constexpr GfxShaderStage GetStage(ShaderId shader)
		{
			switch (shader)
			{
			case VS_Sky:
			case VS_Texture:
			case VS_Solid:
			case VS_Billboard:
			case VS_Sun:
			case VS_Decal:
			case VS_GBufferTerrain:
			case VS_GBufferPBR:
			case VS_FullscreenQuad:
			case VS_LensFlare:
			case VS_Bokeh:
			case VS_Shadow:
			case VS_ShadowTransparent:
			case VS_Ocean:
			case VS_OceanLOD:
			case VS_Foliage:
			case VS_Particle:
				return GfxShaderStage::VS;
			case PS_Skybox:
			case PS_HosekWilkieSky:
			case PS_UniformSky:
			case PS_Texture:
			case PS_Solid:
			case PS_Decal:
			case PS_DecalsModifyNormals:
			case PS_GBufferPBR:
			case PS_GBufferPBR_Mask:
			case PS_GBufferTerrain:
			case PS_AmbientPBR:
			case PS_AmbientPBR_AO:
			case PS_AmbientPBR_IBL:
			case PS_AmbientPBR_AO_IBL:
			case PS_DeferredLighting:
			case PS_ClusterLighting:
			case PS_ToneMap_Reinhard:
			case PS_ToneMap_Linear:
			case PS_ToneMap_Hable:
			case PS_ToneMap_TonyMcMapface:
			case PS_FXAA:
			case PS_TAA:
			case PS_CopyTextures:
			case PS_AddTextures:
			case PS_SSAO:
			case PS_HBAO:
			case PS_SSR:
			case PS_LensFlare:
			case PS_GodRays:
			case PS_DepthOfField:
			case PS_Bokeh:
			case PS_VolumetricClouds:
			case PS_MotionVectors:
			case PS_MotionBlur:
			case PS_Fog:
			case PS_Shadow:
			case PS_ShadowTransparent:
			case PS_VolumetricLight_Directional:
			case PS_VolumetricLight_Spot:
			case PS_VolumetricLight_Point:
			case PS_VolumetricLight_DirectionalWithCascades:
			case PS_Ocean:
			case PS_Foliage:
			case PS_Particle:
			case PS_FilmEffects:
				return GfxShaderStage::PS;
			case GS_LensFlare:
			case GS_Bokeh:
				return GfxShaderStage::GS;
			case CS_BlurHorizontal:
			case CS_BlurVertical:
			case CS_BokehGeneration:
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
			case CS_ParticleInitDeadList:
			case CS_ParticleReset:
			case CS_ParticleEmit:
			case CS_ParticleSimulate:
			case CS_ParticleBitonicSortStep:
			case CS_ParticleSort512:
			case CS_ParticleSortInner512:
			case CS_ParticleInitSortArgs:
			case CS_Picker:
				return GfxShaderStage::CS;
			case HS_OceanLOD:
				return GfxShaderStage::HS;
			case DS_OceanLOD:
				return GfxShaderStage::DS;
			case ShaderId_Count:
			default:
				return GfxShaderStage::StageCount;
			}
		}
		constexpr std::string GetShaderSource(ShaderId shader)
		{
			switch (shader)
			{
			case VS_Sky:
			case PS_Skybox:
				return "Misc/Skybox.hlsl";
			case PS_HosekWilkieSky:
				return "Misc/HosekWilkieSky.hlsl";
			case PS_UniformSky:
				return "Misc/UniformSky.hlsl";
			case VS_Texture:
			case PS_Texture:
				return "Misc/Texture.hlsl";
			case VS_Solid:
			case PS_Solid:
				return "Misc/Solid.hlsl";
			case VS_Sun:
				return "Misc/Sun.hlsl";
			case VS_Billboard:
				return "Misc/Billboard.hlsl";
			case VS_FullscreenQuad:
				return "Postprocess/FullscreenQuad.hlsl";
			case PS_AmbientPBR:
			case PS_AmbientPBR_AO:
			case PS_AmbientPBR_IBL:
			case PS_AmbientPBR_AO_IBL:
				return "Lighting/Ambient.hlsl";
			case PS_DeferredLighting:
				return "Lighting/DeferredLighting.hlsl";
			case PS_ClusterLighting:
				return "Lighting/ClusterLighting.hlsl";
			case PS_ToneMap_Reinhard:
			case PS_ToneMap_Linear:
			case PS_ToneMap_Hable:
			case PS_ToneMap_TonyMcMapface:
				return "Postprocess/ToneMap.hlsl";
			case PS_FXAA:
				return "Postprocess/FXAA.hlsl";
			case PS_TAA:
				return "Postprocess/TAA.hlsl";
			case PS_CopyTextures:
				return "Postprocess/CopyTexture.hlsl";
			case PS_AddTextures:
				return "Postprocess/AddTextures.hlsl";
			case PS_SSAO:
				return "Postprocess/SSAO.hlsl";
			case PS_HBAO:
				return "Postprocess/HBAO.hlsl";
			case PS_SSR:
				return "Postprocess/SSR.hlsl";
			case VS_LensFlare:
			case GS_LensFlare:
			case PS_LensFlare:
				return "Postprocess/LensFlare.hlsl";
			case PS_GodRays:
				return "Postprocess/GodRays.hlsl";
			case PS_DepthOfField:
				return "Postprocess/DepthOfField.hlsl";
			case VS_Bokeh:
			case GS_Bokeh:
			case PS_Bokeh:
				return "Postprocess/Bokeh.hlsl";
			case CS_BokehGeneration:
				return "Postprocess/BokehGeneration.hlsl";
			case PS_VolumetricClouds:
				return "Postprocess/VolumetricClouds.hlsl";
			case PS_MotionVectors:
				return "Postprocess/MotionVectors.hlsl";
			case PS_MotionBlur:
				return "Postprocess/MotionBlur.hlsl";
			case PS_Fog:
				return "Postprocess/Fog.hlsl";
			case PS_FilmEffects:
				return "Postprocess/FilmEffects.hlsl";
			case VS_Shadow:
			case VS_ShadowTransparent:
			case PS_Shadow:
			case PS_ShadowTransparent:
				return "Misc/Shadow.hlsl";
			case PS_VolumetricLight_Directional:
				return "Postprocess/VolumetricLighting_Directional.hlsl";
			case PS_VolumetricLight_DirectionalWithCascades:
				return "Postprocess/VolumetricLighting_Cascades.hlsl";
			case PS_VolumetricLight_Spot:
				return "Postprocess/VolumetricLighting_Spot.hlsl";
			case PS_VolumetricLight_Point:
				return "Postprocess/VolumetricLighting_Point.hlsl";
			case CS_BlurHorizontal:
			case CS_BlurVertical:
				return "Postprocess/Blur.hlsl";
			case CS_BloomExtract:
				return "Postprocess/BloomExtract.hlsl";
			case CS_BloomCombine:
				return "Postprocess/BloomCombine.hlsl";
			case CS_OceanInitialSpectrum:
				return "Ocean/InitialSpectrum.hlsl";
			case CS_OceanPhase:
				return "Ocean/Phase.hlsl";
			case CS_OceanSpectrum:
				return "Ocean/Spectrum.hlsl";
			case CS_OceanFFT_Horizontal:
				return "Ocean/FFT_Horizontal.hlsl";
			case CS_OceanFFT_Vertical:
				return "Ocean/FFT_Vertical.hlsl";
			case CS_OceanNormalMap:
				return "Ocean/NormalMap.hlsl";
			case CS_TiledLighting:
				return "Lighting/TiledLighting.hlsl";
			case CS_ClusterBuilding:
				return "Lighting/ClusterBuilding.hlsl";
			case CS_ClusterCulling:
				return "Lighting/ClusterCulling.hlsl";
			case VS_Ocean:
			case PS_Ocean:
				return "Ocean/Ocean.hlsl";
			case VS_OceanLOD:
			case HS_OceanLOD:
			case DS_OceanLOD:
				return "Ocean/OceanLod.hlsl";
			case VS_GBufferPBR:
			case PS_GBufferPBR:
			case PS_GBufferPBR_Mask:
				return "GBuffer/GBuffer.hlsl";
			case VS_GBufferTerrain:
			case PS_GBufferTerrain:
				return "GBuffer/Terrain.hlsl";
			case VS_Decal:
			case PS_Decal:
			case PS_DecalsModifyNormals:
				return "GBuffer/Decal.hlsl";
			case VS_Foliage:
			case PS_Foliage:
				return "GBuffer/Foliage.hlsl";
			case CS_Picker:
				return "Misc/Picker.hlsl";
			case VS_Particle:
			case PS_Particle:
				return "Particles/Particle.hlsl";
			case CS_ParticleInitDeadList:
				return "Particles/InitDeadList.hlsl";
			case CS_ParticleReset:
				return "Particles/ParticleReset.hlsl";
			case CS_ParticleEmit:
				return "Particles/ParticleEmit.hlsl";
			case CS_ParticleSimulate:
				return "Particles/ParticleSimulate.hlsl";
			case CS_ParticleBitonicSortStep:
				return "Particles/BitonicSortStep.hlsl";
			case CS_ParticleSort512:
				return "Particles/Sort512.hlsl";
			case CS_ParticleSortInner512:
				return "Particles/SortInner512.hlsl";
			case CS_ParticleInitSortArgs:
				return "Particles/InitSortDispatchArgs.hlsl";
			case ShaderId_Count:
			default:
				return "";
			}
		}
		constexpr std::string GetEntryPoint(ShaderId shader)
		{
			switch (shader)
			{
			case VS_Shadow: 
			case VS_ShadowTransparent:
				return "ShadowVS";
			case PS_Shadow: 
			case PS_ShadowTransparent:
				return "ShadowPS";
			case PS_AddTextures:
				return "AddTextures";
			case CS_BloomExtract:
				return "BloomExtract";
			case CS_BloomCombine:
				return "BloomCombine";
			case CS_BlurHorizontal:
				return "BlurHorizontal";
			case CS_BlurVertical:
				return "BlurVertical";
			case VS_Bokeh:
				return "BokehVS";
			case GS_Bokeh:
				return "BokehGS";
			case PS_Bokeh:
				return "BokehPS";
			case CS_BokehGeneration:
				return "BokehGeneration";
			case PS_VolumetricClouds:
				return "VolumetricClouds";
			case PS_CopyTextures:
				return "CopyTexture";
			case PS_DepthOfField:
				return "DepthOfField";
			case PS_Fog:
				return "Fog";
			case PS_FXAA:
				return "FXAA";
			case PS_GodRays:
				return "GodRays";
			case PS_HBAO:
				return "HBAO";
			case PS_SSAO:
				return "SSAO";
			case VS_LensFlare:
				return "LensFlareVS";
			case GS_LensFlare:
				return "LensFlareGS";
			case PS_LensFlare:
				return "LensFlarePS";
			case VS_FullscreenQuad:
				return "FullscreenQuad";
			case PS_ToneMap_Reinhard:
			case PS_ToneMap_Linear:
			case PS_ToneMap_Hable:
			case PS_ToneMap_TonyMcMapface:
				return "ToneMap";
			case PS_MotionVectors:
				return "MotionVectors";
			case PS_MotionBlur:
				return "MotionBlur";
			case PS_TAA:
				return "TAA";
			case PS_SSR:
				return "SSR";
			case PS_FilmEffects:
				return "FilmEffects";
			case PS_VolumetricLight_Directional:
				return "VolumetricLighting_Directional";
			case PS_VolumetricLight_DirectionalWithCascades:
				return "VolumetricLighting_Cascades";
			case PS_VolumetricLight_Spot:
				return "VolumetricLighting_Spot";
			case PS_VolumetricLight_Point:
				return "VolumetricLighting_Point";
			case VS_Billboard:
				return "BillboardVS";
			case VS_Decal:
				return "DecalVS";
			case PS_Decal:
			case PS_DecalsModifyNormals:
				return "DecalPS";
			case VS_Foliage:
				return "FoliageVS";
			case PS_Foliage:
				return "FoliagePS";
			case PS_HosekWilkieSky:
				return "HosekWilkieSky";
			case PS_UniformSky:
				return "UniformSky";
			case CS_Picker:
				return "Picker";
			case VS_Sky:
				return "SkyVS";
			case PS_Skybox:
				return "SkyboxPS";
			case VS_Solid:
				return "SolidVS";
			case PS_Solid:
				return "SolidPS";
			case VS_Sun:
				return "SunVS";
			case VS_Texture:
				return "TextureVS";
			case PS_Texture:
				return "TexturePS";
			case VS_GBufferPBR:
				return "GBufferVS";
			case PS_GBufferPBR:
			case PS_GBufferPBR_Mask:
				return "GBufferPS";
			case VS_GBufferTerrain:
				return "TerrainVS";
			case PS_GBufferTerrain:
				return "TerrainPS";
			case PS_AmbientPBR:
			case PS_AmbientPBR_AO:
			case PS_AmbientPBR_IBL:
			case PS_AmbientPBR_AO_IBL:
				return "AmbientPS";
			case PS_DeferredLighting:
				return "DeferredLightingPS";
			case CS_TiledLighting:
				return "TiledLightingCS";
			case CS_ClusterBuilding:
				return "ClusterBuildingCS";
			case CS_ClusterCulling:
				return "ClusterCullingCS";
			case PS_ClusterLighting:
				return "ClusterLightingPS";
			case CS_ParticleInitDeadList:
				return "InitDeadListCS";
			case CS_ParticleReset:
				return "ParticleResetCS";
			case CS_ParticleEmit:
				return "ParticleEmitCS";
			case CS_ParticleSimulate:
				return "ParticleSimulateCS";
			case CS_ParticleBitonicSortStep:
				return "BitonicSortStepCS";
			case CS_ParticleSort512:
				return "Sort512CS";
			case CS_ParticleSortInner512:
				return "SortInner512CS";
			case CS_ParticleInitSortArgs:
				return "InitSortDispatchArgsCS";
			case VS_Particle:
				return "ParticleVS";
			case PS_Particle:
				return "ParticlePS";
			case VS_Ocean:
				return "OceanVS";
			case PS_Ocean:
				return "OceanPS";
			case VS_OceanLOD:
				return "OceanLodVS";
			case HS_OceanLOD:
				return "OceanLodHS";
			case DS_OceanLOD:
				return "OceanLodDS";
			case CS_OceanInitialSpectrum:
				return "InitialSpectrumCS";
			case CS_OceanPhase:
				return "PhaseCS";
			case CS_OceanSpectrum:
				return "SpectrumCS";
			case CS_OceanFFT_Horizontal:
				return "FFT_HorizontalCS";
			case CS_OceanFFT_Vertical:
				return "FFT_VerticalCS";
			case CS_OceanNormalMap:
				return "NormalMapCS";
			default:
				return "main";
			}
			return "main";
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
			case PS_ToneMap_TonyMcMapface:
				return { {"TONY_MCMAPFACE", "1" } };
			case VS_ShadowTransparent:
			case PS_ShadowTransparent:
				return { {"TRANSPARENT", "1"} };
			case CS_BlurVertical:
				return { { "VERTICAL", "1" } };
			case PS_GBufferPBR_Mask:
				return { { "MASK", "1" } };
			default:
				return {};
			}
		}

		void CompileShader(ShaderId shader, Bool first_compile = false)
		{
			GfxShaderDesc input{ .entrypoint = GetEntryPoint(shader) };
#if _DEBUG
			input.flags = GfxShaderCompilerFlagBit_Debug | GfxShaderCompilerFlagBit_DisableOptimization;
#else
			input.flags = GfxShaderCompilerFlagBit_None;
#endif
			input.source_file = paths::ShaderDir + GetShaderSource(shader);
			input.stage = GetStage(shader);
			input.macros = GetShaderMacros(shader);

			GfxShaderCompileOutput output{};
			Bool result = GfxShaderCompiler::CompileShader(input, output);
			if (!result) return;

			switch (input.stage)
			{
			case GfxShaderStage::VS:
				if(first_compile) vs_shader_map[shader] = std::make_unique<GfxVertexShader>(device, output.shader_bytecode);
				else vs_shader_map[shader]->Recreate(output.shader_bytecode);
				break;
			case GfxShaderStage::PS:
				if (first_compile) ps_shader_map[shader] = std::make_unique<GfxPixelShader>(device, output.shader_bytecode);
				else ps_shader_map[shader]->Recreate(output.shader_bytecode);
				break;
			case GfxShaderStage::HS:
				if (first_compile) hs_shader_map[shader] = std::make_unique<GfxHullShader>(device, output.shader_bytecode);
				else hs_shader_map[shader]->Recreate(output.shader_bytecode);
				break;
			case GfxShaderStage::DS:
				if (first_compile) ds_shader_map[shader] = std::make_unique<GfxDomainShader>(device, output.shader_bytecode);
				else ds_shader_map[shader]->Recreate(output.shader_bytecode);
				break;
			case GfxShaderStage::GS:
				if (first_compile) gs_shader_map[shader] = std::make_unique<GfxGeometryShader>(device, output.shader_bytecode);
				else gs_shader_map[shader]->Recreate(output.shader_bytecode);
				break;
			case GfxShaderStage::CS:
				if (first_compile) cs_shader_map[shader] = std::make_unique<GfxComputeShader>(device, output.shader_bytecode);
				else cs_shader_map[shader]->Recreate(output.shader_bytecode);
				break;
			default:
				ADRIA_ASSERT(false);
			}

			file_shader_map[fs::path(input.source_file)].insert(shader);
			for (auto const& include : output.includes)
			{
				file_shader_map[fs::path(include)].insert(shader);
			}
		}
		void CreateAllPrograms()
		{
			using UnderlyingType = std::underlying_type_t<ShaderId>;
			for (UnderlyingType s = 0; s < ShaderId_Count; ++s)
			{
				ShaderId shader = (ShaderId)s;
				if (GetStage(shader) != GfxShaderStage::VS) continue;
				
				input_layout_map[shader] = std::make_unique<GfxInputLayout>(device, vs_shader_map[shader]->GetBytecode());
			}

			gfx_shader_program_map[ShaderProgram::Skybox].SetVertexShader(vs_shader_map[VS_Sky].get()).SetPixelShader(ps_shader_map[PS_Skybox].get()).SetInputLayout(input_layout_map[VS_Sky].get());
			gfx_shader_program_map[ShaderProgram::HosekWilkieSky].SetVertexShader(vs_shader_map[VS_Sky].get()).SetPixelShader(ps_shader_map[PS_HosekWilkieSky].get()).SetInputLayout(input_layout_map[VS_Sky].get());
			gfx_shader_program_map[ShaderProgram::UniformColorSky].SetVertexShader(vs_shader_map[VS_Sky].get()).SetPixelShader(ps_shader_map[PS_UniformSky].get()).SetInputLayout(input_layout_map[VS_Sky].get());
			gfx_shader_program_map[ShaderProgram::Texture].SetVertexShader(vs_shader_map[VS_Texture].get()).SetPixelShader(ps_shader_map[PS_Texture].get()).SetInputLayout(input_layout_map[VS_Texture].get());
			gfx_shader_program_map[ShaderProgram::Solid].SetVertexShader(vs_shader_map[VS_Solid].get()).SetPixelShader(ps_shader_map[PS_Solid].get()).SetInputLayout(input_layout_map[VS_Solid].get());
			gfx_shader_program_map[ShaderProgram::Sun].SetVertexShader(vs_shader_map[VS_Sun].get()).SetPixelShader(ps_shader_map[PS_Texture].get()).SetInputLayout(input_layout_map[VS_Sun].get());
			gfx_shader_program_map[ShaderProgram::Billboard].SetVertexShader(vs_shader_map[VS_Billboard].get()).SetPixelShader(ps_shader_map[PS_Texture].get()).SetInputLayout(input_layout_map[VS_Billboard].get());
			gfx_shader_program_map[ShaderProgram::Decals].SetVertexShader(vs_shader_map[VS_Decal].get()).SetPixelShader(ps_shader_map[PS_Decal].get()).SetInputLayout(input_layout_map[VS_Decal].get());
			gfx_shader_program_map[ShaderProgram::Decals_ModifyNormals].SetVertexShader(vs_shader_map[VS_Decal].get()).SetPixelShader(ps_shader_map[PS_DecalsModifyNormals].get()).SetInputLayout(input_layout_map[VS_Decal].get());
			gfx_shader_program_map[ShaderProgram::GBuffer_Terrain].SetVertexShader(vs_shader_map[VS_GBufferTerrain].get()).SetPixelShader(ps_shader_map[PS_GBufferTerrain].get()).SetInputLayout(input_layout_map[VS_GBufferTerrain].get());
			gfx_shader_program_map[ShaderProgram::GBufferPBR].SetVertexShader(vs_shader_map[VS_GBufferPBR].get()).SetPixelShader(ps_shader_map[PS_GBufferPBR].get()).SetInputLayout(input_layout_map[VS_GBufferPBR].get());
			gfx_shader_program_map[ShaderProgram::GBufferPBR_Mask].SetVertexShader(vs_shader_map[VS_GBufferPBR].get()).SetPixelShader(ps_shader_map[PS_GBufferPBR_Mask].get()).SetInputLayout(input_layout_map[VS_GBufferPBR].get());
			gfx_shader_program_map[ShaderProgram::AmbientPBR].SetVertexShader(vs_shader_map[VS_FullscreenQuad].get()).SetPixelShader(ps_shader_map[PS_AmbientPBR].get()).SetInputLayout(input_layout_map[VS_FullscreenQuad].get());
			gfx_shader_program_map[ShaderProgram::AmbientPBR_AO].SetVertexShader(vs_shader_map[VS_FullscreenQuad].get()).SetPixelShader(ps_shader_map[PS_AmbientPBR_AO].get()).SetInputLayout(input_layout_map[VS_FullscreenQuad].get());
			gfx_shader_program_map[ShaderProgram::AmbientPBR_IBL].SetVertexShader(vs_shader_map[VS_FullscreenQuad].get()).SetPixelShader(ps_shader_map[PS_AmbientPBR_IBL].get()).SetInputLayout(input_layout_map[VS_FullscreenQuad].get());
			gfx_shader_program_map[ShaderProgram::AmbientPBR_AO_IBL].SetVertexShader(vs_shader_map[VS_FullscreenQuad].get()).SetPixelShader(ps_shader_map[PS_AmbientPBR_AO_IBL].get()).SetInputLayout(input_layout_map[VS_FullscreenQuad].get());
			gfx_shader_program_map[ShaderProgram::LightingPBR].SetVertexShader(vs_shader_map[VS_FullscreenQuad].get()).SetPixelShader(ps_shader_map[PS_DeferredLighting].get()).SetInputLayout(input_layout_map[VS_FullscreenQuad].get());
			gfx_shader_program_map[ShaderProgram::ClusterLightingPBR].SetVertexShader(vs_shader_map[VS_FullscreenQuad].get()).SetPixelShader(ps_shader_map[PS_ClusterLighting].get()).SetInputLayout(input_layout_map[VS_FullscreenQuad].get());
			gfx_shader_program_map[ShaderProgram::ToneMap_Reinhard].SetVertexShader(vs_shader_map[VS_FullscreenQuad].get()).SetPixelShader(ps_shader_map[PS_ToneMap_Reinhard].get()).SetInputLayout(input_layout_map[VS_FullscreenQuad].get());
			gfx_shader_program_map[ShaderProgram::ToneMap_Linear].SetVertexShader(vs_shader_map[VS_FullscreenQuad].get()).SetPixelShader(ps_shader_map[PS_ToneMap_Linear].get()).SetInputLayout(input_layout_map[VS_FullscreenQuad].get());
			gfx_shader_program_map[ShaderProgram::ToneMap_Hable].SetVertexShader(vs_shader_map[VS_FullscreenQuad].get()).SetPixelShader(ps_shader_map[PS_ToneMap_Hable].get()).SetInputLayout(input_layout_map[VS_FullscreenQuad].get());
			gfx_shader_program_map[ShaderProgram::ToneMap_TonyMcMapface].SetVertexShader(vs_shader_map[VS_FullscreenQuad].get()).SetPixelShader(ps_shader_map[PS_ToneMap_TonyMcMapface].get()).SetInputLayout(input_layout_map[VS_FullscreenQuad].get());
			gfx_shader_program_map[ShaderProgram::FilmEffects].SetVertexShader(vs_shader_map[VS_FullscreenQuad].get()).SetPixelShader(ps_shader_map[PS_FilmEffects].get()).SetInputLayout(input_layout_map[VS_FullscreenQuad].get());

			gfx_shader_program_map[ShaderProgram::FXAA].SetVertexShader(vs_shader_map[VS_FullscreenQuad].get()).SetPixelShader(ps_shader_map[PS_FXAA].get()).SetInputLayout(input_layout_map[VS_FullscreenQuad].get());
			gfx_shader_program_map[ShaderProgram::TAA].SetVertexShader(vs_shader_map[VS_FullscreenQuad].get()).SetPixelShader(ps_shader_map[PS_TAA].get()).SetInputLayout(input_layout_map[VS_FullscreenQuad].get());
			gfx_shader_program_map[ShaderProgram::Copy].SetVertexShader(vs_shader_map[VS_FullscreenQuad].get()).SetPixelShader(ps_shader_map[PS_CopyTextures].get()).SetInputLayout(input_layout_map[VS_FullscreenQuad].get());
			gfx_shader_program_map[ShaderProgram::Add].SetVertexShader(vs_shader_map[VS_FullscreenQuad].get()).SetPixelShader(ps_shader_map[PS_AddTextures].get()).SetInputLayout(input_layout_map[VS_FullscreenQuad].get());
			gfx_shader_program_map[ShaderProgram::SSAO].SetVertexShader(vs_shader_map[VS_FullscreenQuad].get()).SetPixelShader(ps_shader_map[PS_SSAO].get()).SetInputLayout(input_layout_map[VS_FullscreenQuad].get());
			gfx_shader_program_map[ShaderProgram::HBAO].SetVertexShader(vs_shader_map[VS_FullscreenQuad].get()).SetPixelShader(ps_shader_map[PS_HBAO].get()).SetInputLayout(input_layout_map[VS_FullscreenQuad].get());
			gfx_shader_program_map[ShaderProgram::SSR].SetVertexShader(vs_shader_map[VS_FullscreenQuad].get()).SetPixelShader(ps_shader_map[PS_SSR].get()).SetInputLayout(input_layout_map[VS_FullscreenQuad].get());
			gfx_shader_program_map[ShaderProgram::GodRays].SetVertexShader(vs_shader_map[VS_FullscreenQuad].get()).SetPixelShader(ps_shader_map[PS_GodRays].get()).SetInputLayout(input_layout_map[VS_FullscreenQuad].get());
			gfx_shader_program_map[ShaderProgram::DOF].SetVertexShader(vs_shader_map[VS_FullscreenQuad].get()).SetPixelShader(ps_shader_map[PS_DepthOfField].get()).SetInputLayout(input_layout_map[VS_FullscreenQuad].get());
			gfx_shader_program_map[ShaderProgram::LensFlare].SetVertexShader(vs_shader_map[VS_LensFlare].get()).SetGeometryShader(gs_shader_map[GS_LensFlare].get()).SetPixelShader(ps_shader_map[PS_LensFlare].get()).SetInputLayout(input_layout_map[VS_LensFlare].get());
			gfx_shader_program_map[ShaderProgram::BokehDraw].SetVertexShader(vs_shader_map[VS_Bokeh].get()).SetGeometryShader(gs_shader_map[GS_Bokeh].get()).SetPixelShader(ps_shader_map[PS_Bokeh].get()).SetInputLayout(input_layout_map[VS_Bokeh].get());

			gfx_shader_program_map[ShaderProgram::Volumetric_Clouds].SetVertexShader(vs_shader_map[VS_FullscreenQuad].get()).SetPixelShader(ps_shader_map[PS_VolumetricClouds].get()).SetInputLayout(input_layout_map[VS_FullscreenQuad].get());
			gfx_shader_program_map[ShaderProgram::MotionVectors].SetVertexShader(vs_shader_map[VS_FullscreenQuad].get()).SetPixelShader(ps_shader_map[PS_MotionVectors].get()).SetInputLayout(input_layout_map[VS_FullscreenQuad].get());
			gfx_shader_program_map[ShaderProgram::MotionBlur].SetVertexShader(vs_shader_map[VS_FullscreenQuad].get()).SetPixelShader(ps_shader_map[PS_MotionBlur].get()).SetInputLayout(input_layout_map[VS_FullscreenQuad].get());
			gfx_shader_program_map[ShaderProgram::Fog].SetVertexShader(vs_shader_map[VS_FullscreenQuad].get()).SetPixelShader(ps_shader_map[PS_Fog].get()).SetInputLayout(input_layout_map[VS_FullscreenQuad].get());

			gfx_shader_program_map[ShaderProgram::DepthMap].SetVertexShader(vs_shader_map[VS_Shadow].get()).SetPixelShader(ps_shader_map[PS_Shadow].get()).SetInputLayout(input_layout_map[VS_Shadow].get());
			gfx_shader_program_map[ShaderProgram::DepthMap_Transparent].SetVertexShader(vs_shader_map[VS_ShadowTransparent].get()).SetPixelShader(ps_shader_map[PS_ShadowTransparent].get()).SetInputLayout(input_layout_map[VS_ShadowTransparent].get());

			gfx_shader_program_map[ShaderProgram::Volumetric_Directional].SetVertexShader(vs_shader_map[VS_FullscreenQuad].get()).SetPixelShader(ps_shader_map[PS_VolumetricLight_Directional].get()).SetInputLayout(input_layout_map[VS_FullscreenQuad].get());
			gfx_shader_program_map[ShaderProgram::Volumetric_DirectionalCascades].SetVertexShader(vs_shader_map[VS_FullscreenQuad].get()).SetPixelShader(ps_shader_map[PS_VolumetricLight_DirectionalWithCascades].get()).SetInputLayout(input_layout_map[VS_FullscreenQuad].get());
			gfx_shader_program_map[ShaderProgram::Volumetric_Spot].SetVertexShader(vs_shader_map[VS_FullscreenQuad].get()).SetPixelShader(ps_shader_map[PS_VolumetricLight_Spot].get()).SetInputLayout(input_layout_map[VS_FullscreenQuad].get());
			gfx_shader_program_map[ShaderProgram::Volumetric_Point].SetVertexShader(vs_shader_map[VS_FullscreenQuad].get()).SetPixelShader(ps_shader_map[PS_VolumetricLight_Point].get()).SetInputLayout(input_layout_map[VS_FullscreenQuad].get());

			compute_shader_program_map[ShaderProgram::Blur_Horizontal].SetComputeShader(cs_shader_map[CS_BlurHorizontal].get()); 
			compute_shader_program_map[ShaderProgram::Blur_Vertical].SetComputeShader(cs_shader_map[CS_BlurVertical].get()); 
			compute_shader_program_map[ShaderProgram::BokehGenerate].SetComputeShader(cs_shader_map[CS_BokehGeneration].get()); 
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

			gfx_shader_program_map[ShaderProgram::Ocean].SetVertexShader(vs_shader_map[VS_Ocean].get()).SetPixelShader(ps_shader_map[PS_Ocean].get()).SetInputLayout(input_layout_map[VS_Ocean].get());
			gfx_shader_program_map[ShaderProgram::OceanLOD].SetVertexShader(vs_shader_map[VS_OceanLOD].get()).SetHullShader(hs_shader_map[HS_OceanLOD].get()).SetDomainShader(ds_shader_map[DS_OceanLOD].get()).
				SetPixelShader(ps_shader_map[PS_Ocean].get()).SetInputLayout(input_layout_map[VS_OceanLOD].get());
				
			compute_shader_program_map[ShaderProgram::Picker].SetComputeShader(cs_shader_map[CS_Picker].get()); 

			compute_shader_program_map[ShaderProgram::ParticleInitDeadList].SetComputeShader(cs_shader_map[CS_ParticleInitDeadList].get()); 
			compute_shader_program_map[ShaderProgram::ParticleReset].SetComputeShader(cs_shader_map[CS_ParticleReset].get()); 
			compute_shader_program_map[ShaderProgram::ParticleEmit].SetComputeShader(cs_shader_map[CS_ParticleEmit].get()); 
			compute_shader_program_map[ShaderProgram::ParticleSimulate].SetComputeShader(cs_shader_map[CS_ParticleSimulate].get()); 
			compute_shader_program_map[ShaderProgram::ParticleBitonicSortStep].SetComputeShader(cs_shader_map[CS_ParticleBitonicSortStep].get()); 
			compute_shader_program_map[ShaderProgram::ParticleSort512].SetComputeShader(cs_shader_map[CS_ParticleSort512].get()); 
			compute_shader_program_map[ShaderProgram::ParticleSortInner512].SetComputeShader(cs_shader_map[CS_ParticleSortInner512].get()); 
			compute_shader_program_map[ShaderProgram::ParticleSortInitArgs].SetComputeShader(cs_shader_map[CS_ParticleInitSortArgs].get()); 
			gfx_shader_program_map[ShaderProgram::Particles].SetVertexShader(vs_shader_map[VS_Particle].get()).SetPixelShader(ps_shader_map[PS_Particle].get()).SetInputLayout(input_layout_map[VS_Particle].get());
			gfx_shader_program_map[ShaderProgram::GBuffer_Foliage].SetVertexShader(vs_shader_map[VS_Foliage].get()).SetPixelShader(ps_shader_map[PS_Foliage].get()).SetInputLayout(input_layout_map[VS_Foliage].get());
		}
		void CompileAllShaders()
		{
			Timer t;
			ADRIA_LOG(INFO, "Compiling all shaders...");
			using UnderlyingType = std::underlying_type_t<ShaderId>;

			std::vector<UnderlyingType> shaders(ShaderId_Count);
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
			for (ShaderId const& shader_id : file_shader_map[fs::path(filename)])
			{
				CompileShader(shader_id);
			}
		}
	}

	void ShaderManager::Initialize(GfxDevice* _device)
	{
		device = _device;
		file_watcher = std::make_unique<FileWatcher>();
		CompileAllShaders();
		file_watcher->AddPathToWatch(paths::ShaderDir);
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
		Bool is_gfx_program = gfx_shader_program_map.contains(shader_program);
		if (is_gfx_program) return &gfx_shader_program_map[shader_program];
		else return &compute_shader_program_map[shader_program];
	}
	void ShaderManager::CheckIfShadersHaveChanged()
	{
		file_watcher->CheckWatchedFiles();
	}
}

