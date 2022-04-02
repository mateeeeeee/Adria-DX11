
#include <unordered_map>
#include <memory>
#include <string_view>
#include "ShaderCache.h"
#include "../Graphics/ShaderProgram.h"
#include "../Graphics/ShaderCompiler.h"

namespace adria
{

	namespace
	{
		ID3D11Device* device;
		std::unordered_map<EShader, ShaderBlob> shader_map;
		std::unordered_map<EShaderProgram, std::unique_ptr<ShaderProgram>> shader_program_map;
		std::unordered_map<EShader, std::vector<ShaderProgram*>> dependent_programs;

		//use std::filesystem?
		constexpr std::string_view compiled_shaders_directory = "Resources/Compiled Shaders/";
		constexpr std::string_view shaders_directory = "Resources/Shaders/";
		constexpr std::string_view shaders_headers_directories[] = { "Resources/Shaders/Globals", "Resources/Shaders/Util/" };

		void CreateAllPrograms()
		{
			shader_program_map[EShaderProgram::Skybox] = std::make_unique<StandardProgram>(device, shader_map[VS_Sky], shader_map[PS_Skybox]);
			shader_program_map[EShaderProgram::HosekWilkieSky] = std::make_unique<StandardProgram>(device, shader_map[VS_Sky], shader_map[PS_HosekWilkieSky]);
			shader_program_map[EShaderProgram::UniformColorSky] = std::make_unique<StandardProgram>(device, shader_map[VS_Sky], shader_map[PS_UniformColorSky]);
			shader_program_map[EShaderProgram::Texture] = std::make_unique<StandardProgram>(device, shader_map[VS_Texture], shader_map[PS_Texture]);
			shader_program_map[EShaderProgram::Solid] = std::make_unique<StandardProgram>(device, shader_map[VS_Solid], shader_map[PS_Solid]);
			shader_program_map[EShaderProgram::Sun] = std::make_unique<StandardProgram>(device, shader_map[VS_Sun], shader_map[PS_Sun]);
			shader_program_map[EShaderProgram::Billboard] = std::make_unique<StandardProgram>(device, shader_map[VS_Billboard], shader_map[PS_Billboard]);
			shader_program_map[EShaderProgram::Decals] = std::make_unique<StandardProgram>(device, shader_map[VS_Decals], shader_map[PS_Decals]);
			shader_program_map[EShaderProgram::Decals_ModifyNormals] = std::make_unique<StandardProgram>(device, shader_map[VS_Decals], shader_map[PS_DecalsModifyNormals]);
			shader_program_map[EShaderProgram::GBuffer_Terrain] = std::make_unique<StandardProgram>(device, shader_map[VS_GBufferTerrain], shader_map[PS_GBufferTerrain]);
			shader_program_map[EShaderProgram::GBufferPBR] = std::make_unique<StandardProgram>(device, shader_map[VS_GBufferPBR], shader_map[PS_GBufferPBR]);
			shader_program_map[EShaderProgram::AmbientPBR] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_AmbientPBR]);
			shader_program_map[EShaderProgram::AmbientPBR_AO] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_AmbientPBR_AO]);
			shader_program_map[EShaderProgram::AmbientPBR_IBL] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_AmbientPBR_IBL]);
			shader_program_map[EShaderProgram::AmbientPBR_AO_IBL] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_AmbientPBR_AO_IBL]);
			shader_program_map[EShaderProgram::LightingPBR] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_LightingPBR]);
			shader_program_map[EShaderProgram::ClusterLightingPBR] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_ClusterLightingPBR]);
			shader_program_map[EShaderProgram::ToneMap_Reinhard] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_ToneMap_Reinhard]);
			shader_program_map[EShaderProgram::ToneMap_Linear] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_ToneMap_Linear]);
			shader_program_map[EShaderProgram::ToneMap_Hable] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_ToneMap_Hable]);

			shader_program_map[EShaderProgram::FXAA] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_FXAA]);
			shader_program_map[EShaderProgram::TAA] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_TAA]);
			shader_program_map[EShaderProgram::Copy] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_Copy]);
			shader_program_map[EShaderProgram::Add] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_Add]);
			shader_program_map[EShaderProgram::SSAO] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_SSAO]);
			shader_program_map[EShaderProgram::HBAO] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_HBAO]);
			shader_program_map[EShaderProgram::SSR] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_SSR]);
			shader_program_map[EShaderProgram::GodRays] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_GodRays]);
			shader_program_map[EShaderProgram::DOF] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_DepthOfField]);
			shader_program_map[EShaderProgram::LensFlare] = std::make_unique<GeometryProgram>(device, shader_map[VS_LensFlare], shader_map[GS_LensFlare], shader_map[PS_LensFlare]);
			shader_program_map[EShaderProgram::BokehDraw] = std::make_unique<GeometryProgram>(device, shader_map[VS_Bokeh], shader_map[GS_Bokeh], shader_map[PS_Bokeh]);

			shader_program_map[EShaderProgram::Volumetric_Clouds] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_VolumetricClouds]);
			shader_program_map[EShaderProgram::VelocityBuffer] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_VelocityBuffer]);
			shader_program_map[EShaderProgram::MotionBlur] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_MotionBlur]);
			shader_program_map[EShaderProgram::Fog] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_Fog]);

			shader_program_map[EShaderProgram::DepthMap] = std::make_unique<StandardProgram>(device, shader_map[VS_DepthMap], shader_map[PS_DepthMap]);
			shader_program_map[EShaderProgram::DepthMap_Transparent] = std::make_unique<StandardProgram>(device, shader_map[VS_DepthMapTransparent], shader_map[PS_DepthMapTransparent]);

			shader_program_map[EShaderProgram::Volumetric_Directional] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_VolumetricLight_Directional]);
			shader_program_map[EShaderProgram::Volumetric_DirectionalCascades] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_VolumetricLight_DirectionalWithCascades]);
			shader_program_map[EShaderProgram::Volumetric_Spot] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_VolumetricLight_Spot]);
			shader_program_map[EShaderProgram::Volumetric_Point] = std::make_unique<StandardProgram>(device, shader_map[VS_ScreenQuad], shader_map[PS_VolumetricLight_Point]);

			shader_program_map[EShaderProgram::Blur_Horizontal] = std::make_unique<ComputeProgram>(device, shader_map[CS_BlurHorizontal]);
			shader_program_map[EShaderProgram::Blur_Vertical] = std::make_unique<ComputeProgram>(device, shader_map[CS_BlurVertical]);
			shader_program_map[EShaderProgram::BokehGenerate] = std::make_unique<ComputeProgram>(device, shader_map[CS_BokehGenerate]);
			shader_program_map[EShaderProgram::BloomExtract] = std::make_unique<ComputeProgram>(device, shader_map[CS_BloomExtract]);
			shader_program_map[EShaderProgram::BloomCombine] = std::make_unique<ComputeProgram>(device, shader_map[CS_BloomCombine]);

			shader_program_map[EShaderProgram::OceanInitialSpectrum] = std::make_unique<ComputeProgram>(device, shader_map[CS_OceanInitialSpectrum]);
			shader_program_map[EShaderProgram::OceanPhase] = std::make_unique<ComputeProgram>(device, shader_map[CS_OceanPhase]);
			shader_program_map[EShaderProgram::OceanSpectrum] = std::make_unique<ComputeProgram>(device, shader_map[CS_OceanSpectrum]);
			shader_program_map[EShaderProgram::OceanNormalMap] = std::make_unique<ComputeProgram>(device, shader_map[CS_OceanNormalMap]);
			shader_program_map[EShaderProgram::OceanFFT_Horizontal] = std::make_unique<ComputeProgram>(device, shader_map[CS_OceanFFT_Horizontal]);
			shader_program_map[EShaderProgram::OceanFFT_Vertical] = std::make_unique<ComputeProgram>(device, shader_map[CS_OceanFFT_Vertical]);

			shader_program_map[EShaderProgram::TiledLighting] = std::make_unique<ComputeProgram>(device, shader_map[CS_TiledLighting]);
			shader_program_map[EShaderProgram::ClusterBuilding] = std::make_unique<ComputeProgram>(device, shader_map[CS_ClusterBuilding]);
			shader_program_map[EShaderProgram::ClusterCulling] = std::make_unique<ComputeProgram>(device, shader_map[CS_ClusterCulling]);
			shader_program_map[EShaderProgram::VoxelCopy] = std::make_unique<ComputeProgram>(device, shader_map[CS_VoxelCopy]);
			shader_program_map[EShaderProgram::VoxelSecondBounce] = std::make_unique<ComputeProgram>(device, shader_map[CS_VoxelSecondBounce]);

			shader_program_map[EShaderProgram::Ocean] = std::make_unique<StandardProgram>(device, shader_map[VS_Ocean], shader_map[PS_Ocean]);
			shader_program_map[EShaderProgram::OceanLOD] = std::make_unique<TessellationProgram>(device, shader_map[VS_OceanLOD], 
				shader_map[HS_OceanLOD], shader_map[DS_OceanLOD], shader_map[PS_OceanLOD]);

			shader_program_map[EShaderProgram::Voxelize] = std::make_unique<GeometryProgram>(device, shader_map[VS_Voxelize], shader_map[GS_Voxelize], shader_map[PS_Voxelize]);
			shader_program_map[EShaderProgram::VoxelizeDebug] = std::make_unique<GeometryProgram>(device, shader_map[VS_VoxelizeDebug], shader_map[GS_VoxelizeDebug], shader_map[PS_VoxelizeDebug]);

			shader_program_map[EShaderProgram::Picker] = std::make_unique<ComputeProgram>(device, shader_map[CS_Picker]);

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

			shader_program_map[EShaderProgram::ParticleInitDeadList] = std::make_unique<ComputeProgram>(device, shader_map[CS_ParticleInitDeadList]);
			shader_program_map[EShaderProgram::ParticleReset] = std::make_unique<ComputeProgram>(device, shader_map[CS_ParticleReset]);
			shader_program_map[EShaderProgram::ParticleEmit] = std::make_unique<ComputeProgram>(device, shader_map[CS_ParticleEmit]);
			shader_program_map[EShaderProgram::ParticleSimulate] = std::make_unique<ComputeProgram>(device, shader_map[CS_ParticleSimulate]);
			shader_program_map[EShaderProgram::ParticleBitonicSortStep] = std::make_unique<ComputeProgram>(device, shader_map[CS_ParticleBitonicSortStep]);
			shader_program_map[EShaderProgram::ParticleSort512] = std::make_unique<ComputeProgram>(device, shader_map[CS_ParticleSort512]);
			shader_program_map[EShaderProgram::ParticleSortInner512] = std::make_unique<ComputeProgram>(device, shader_map[CS_ParticleSortInner512]);
			shader_program_map[EShaderProgram::ParticleSortInitArgs] = std::make_unique<ComputeProgram>(device, shader_map[CS_ParticleSortInitArgs]);
			shader_program_map[EShaderProgram::Particles] = std::make_unique<StandardProgram>(device, shader_map[VS_Particles], shader_map[PS_Particles]);

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
				shader_info.defines = { {"DECAL_MODIFY_NORMALS", ""} };

				ShaderCompiler::CompileShader(shader_info, shader_map[PS_DecalsModifyNormals]);

				shader_info.shadersource = "Resources/Shaders/Deferred/GeometryPassPBR_VS.hlsl";
				shader_info.stage = EShaderStage::VS;
				shader_info.defines = {};
				ShaderCompiler::CompileShader(shader_info, shader_map[VS_GBufferPBR]);

				shader_info.shadersource = "Resources/Shaders/Deferred/GeometryPassPBR_PS.hlsl";
				shader_info.stage = EShaderStage::PS;
				ShaderCompiler::CompileShader(shader_info, shader_map[PS_GBufferPBR]);

				shader_info.shadersource = "Resources/Shaders/Deferred/AmbientPBR_PS.hlsl";
				shader_info.stage = EShaderStage::PS;
				ShaderCompiler::CompileShader(shader_info, shader_map[PS_AmbientPBR]);

				shader_info.defines = { {"SSAO", "1"} };
				ShaderCompiler::CompileShader(shader_info, shader_map[PS_AmbientPBR_AO]);

				shader_info.defines = { {"IBL", "1"} };
				ShaderCompiler::CompileShader(shader_info, shader_map[PS_AmbientPBR_IBL]);

				shader_info.defines = { {"SSAO", "1"}, {"IBL", "1"} };
				ShaderCompiler::CompileShader(shader_info, shader_map[PS_AmbientPBR_AO_IBL]);

				shader_info.shadersource = "Resources/Shaders/Postprocess/ToneMapPS.hlsl";
				shader_info.stage = EShaderStage::PS;
				shader_info.defines = { {"REINHARD", "1" } };
				ShaderCompiler::CompileShader(shader_info, shader_map[PS_ToneMap_Reinhard]);
				shader_info.defines = { {"LINEAR", "1" } };
				ShaderCompiler::CompileShader(shader_info, shader_map[PS_ToneMap_Linear]);
				shader_info.defines = { {"HABLE", "1" } };
				ShaderCompiler::CompileShader(shader_info, shader_map[PS_ToneMap_Hable]);

				shader_info.shadersource = "Resources/Shaders/Shadows/DepthMapVS.hlsl";
				shader_info.stage = EShaderStage::VS;
				shader_info.defines = {};
				ShaderCompiler::CompileShader(shader_info, shader_map[VS_DepthMap]);
				shader_info.defines = { {"TRANSPARENT", "1"} };
				ShaderCompiler::CompileShader(shader_info, shader_map[VS_DepthMapTransparent]);

				shader_info.shadersource = "Resources/Shaders/Shadows/DepthMapPS.hlsl";
				shader_info.stage = EShaderStage::PS;
				shader_info.defines = {};
				ShaderCompiler::CompileShader(shader_info, shader_map[PS_DepthMap]);
				shader_info.defines = { {"TRANSPARENT", "1"} };
				ShaderCompiler::CompileShader(shader_info, shader_map[PS_DepthMapTransparent]);

				shader_info.shadersource = "Resources/Shaders/Postprocess/BlurCS.hlsl";
				shader_info.stage = EShaderStage::CS;
				shader_info.defines = {};
				ShaderCompiler::CompileShader(shader_info, shader_map[CS_BlurHorizontal]);
				shader_info.defines = { { "VERTICAL", "1" } };
				ShaderCompiler::CompileShader(shader_info, shader_map[CS_BlurVertical]);
			}

			CreateAllPrograms();
		}
	}

	void ShaderCache::Initialize(ID3D11Device* _device)
	{
		device = _device;
		CompileAllShaders();
	}

	
	void ShaderCache::RecompileShader(EShader shader)
	{

	}

	ShaderProgram* ShaderCache::GetShaderProgram(EShaderProgram shader_program)
	{
		return shader_program_map[shader_program].get();
	}

	void ShaderCache::RecompileChangedShaders()
	{

	}

}

