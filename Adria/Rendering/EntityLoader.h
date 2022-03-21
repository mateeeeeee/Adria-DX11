#pragma once

#include <DirectXMath.h>
#include <optional>
#include <functional>
#include <array>
#include <vector>
#include "Components.h"
#include "../Core/Definitions.h"
#include "../tecs/registry.h"
#include "../Math/ComputeNormals.h"
#include "../Utilities/Heightmap.h"

struct ID3D11Device;

namespace adria
{

    enum class ELightMesh
    {
        
        NoMesh,
        Quad,
        Cube
    };
	enum class EFoliageMesh
	{
		SingleQuad,
		DoubleQuad,
		TripleQuad
	};
    enum class ETreeType
    {
        Tree01,
        Tree02
    };

    struct light_parameters_t
    {
        Light light_data;
        ELightMesh mesh_type = ELightMesh::NoMesh;
        uint32 mesh_size = 0u;
        std::optional<std::string> light_texture = std::nullopt;
    };
	struct model_parameters_t
	{
        std::string model_path = "";
        std::string textures_path = "";
        DirectX::XMMATRIX model_matrix = DirectX::XMMatrixIdentity();
	};
    struct skybox_parameters_t
    {
        std::optional<std::wstring> cubemap;
        std::array<std::string, 6> cubemap_textures;
    };
    struct grid_parameters_t
    {
        uint64 tile_count_x;
        uint64 tile_count_z;
        float32 tile_size_x;
        float32 tile_size_z;
        float32 texture_scale_x;
        float32 texture_scale_z;
        uint64 chunk_count_x;
        uint64 chunk_count_z;
        bool split_to_chunks = false;
        ENormalCalculation normal_type = ENormalCalculation::None;
        std::unique_ptr<Heightmap> heightmap = nullptr;
        DirectX::XMFLOAT3 grid_offset = DirectX::XMFLOAT3(0,0,0);
    };
    struct ocean_parameters_t
    {
        grid_parameters_t ocean_grid;
    };
    struct foliage_parameters_t
    {
        std::pair<EFoliageMesh, std::string> mesh_texture_pair;
        int32 foliage_count;
        float32 foliage_scale;
        DirectX::XMFLOAT2 foliage_center;
        DirectX::XMFLOAT2 foliage_extents;
        float32 foliage_height_start;
		float32 foliage_height_end;
		float32 foliage_slope_start;
    };
    struct tree_parameters_t
    {
        ETreeType tree_type;
		int32 tree_count;
		float32 tree_scale;
		DirectX::XMFLOAT2 tree_center;
		DirectX::XMFLOAT2 tree_extents;
        float32 tree_height_start;
		float32 tree_height_end;
		float32 tree_slope_start;
    };
	struct terrain_texture_layer_parameters_t
	{
		float32 terrain_sand_start = -100.0f;
		float32 terrain_sand_end = 0.0f;
		float32 terrain_grass_start = 0.0f;
		float32 terrain_grass_end = 300.0f;
		float32 terrain_slope_grass_start = 0.92f;
		float32 terrain_slope_rocks_start = 0.85f;
		float32 terrain_rocks_start = 50.0f;
        float32 height_mix_zone = 50.0f;
        float32 slope_mix_zone = 0.025f;
	};
	struct terrain_parameters_t
	{
		grid_parameters_t terrain_grid;
        terrain_texture_layer_parameters_t layer_params;
		std::string grass_texture;
		std::string base_texture;
		std::string rock_texture;
		std::string sand_texture;
        std::string layer_texture;
	};
    struct emitter_parameters_t
    {
        std::string name = "Emitter";
        float32 position[3] = { 50.0f, 10.0f, 0.0f};
        float32 velocity[3] = { 0.0f, 7.0f, 0.0f};
        float32 position_variance[3] = {4.0f, 0.0f, 4.0f};
        float32 velocity_variance = { 0.6f };
        float32 lifespan = 50.0f;
        float32 start_size = 22.0f;
        float32 end_size = 5.0f;
        float32 mass = 0.0003f;
        float32 particles_per_second = 100.0f;
        std::string texture_path = "Resources/Textures/Particles/fire.png";
        bool blend = true;
        bool collisions = false;
        int32 collision_thickness = 40;
        bool sort = false;
    };

    struct decal_parameters_t
    {
        std::string name = "Decal";
        std::string albedo_texture_path = "Resources/Decals/Decal_00_Albedo.tga";
        std::string normal_texture_path = "Resources/Decals/Decal_00_Normal.png";
        float32 rotation = 0.0f;
        float32 size = 50.0f;
        EDecalType decal_type = EDecalType::Wall;
        DirectX::XMFLOAT4 position;
        DirectX::XMFLOAT4 normal;
    };
   
    class TextureManager;

	class EntityLoader
	{

	public:
        
        EntityLoader(tecs::registry& reg, ID3D11Device* device, TextureManager& texture_manager);

        [[maybe_unused]] std::vector<tecs::entity> LoadGLTFModel(model_parameters_t const&);

        [[maybe_unused]] tecs::entity LoadSkybox(skybox_parameters_t const&);

        [[maybe_unused]] tecs::entity LoadLight(light_parameters_t const&);

        [[maybe_unused]] std::vector<tecs::entity> LoadOcean(ocean_parameters_t const&);

		[[maybe_unused]] std::vector<tecs::entity> LoadTerrain(terrain_parameters_t&);

		[[maybe_unused]] tecs::entity LoadFoliage(foliage_parameters_t const&);

        [[maybe_unused]] std::vector<tecs::entity> LoadTrees(tree_parameters_t const&);

        [[maybe_unused]] tecs::entity LoadEmitter(emitter_parameters_t const&);

        [[maybe_unused]] tecs::entity LoadDecal(decal_parameters_t const&);

	private:
        tecs::registry& reg;
		TextureManager& texture_manager;
        ID3D11Device* device;

    private:

		[[nodiscard]]
		std::vector<tecs::entity> LoadObjMesh(std::string const& model_path, std::vector<std::string>* diffuse_textures_out = nullptr);

		[[nodiscard]]
		std::vector<tecs::entity> LoadGrid(grid_parameters_t const& args, std::vector<TexturedNormalVertex>* vertices = nullptr);

	};
}


