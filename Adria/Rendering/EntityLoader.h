#pragma once
#pragma comment(lib, "assimp-vc142-mt.lib")
#include <DirectXMath.h>
#include <optional>
#include <functional>
#include <array>
#include <vector>
#include "Components.h"
#include "../Core/Definitions.h"
#include "../tecs/registry.h"
#include "../Math/ComputeNormals.h"

struct ID3D11Device;

namespace adria
{
	struct model_parameters_t
	{
        std::string model_path = "";
        std::string textures_path = "";
        DirectX::XMMATRIX model = DirectX::XMMatrixIdentity();
	};
    struct skybox_parameters_t
    {
        std::optional<std::wstring> cubemap;
        std::array<std::string, 6> cubemap_textures;
    };
    struct grid_parameters_t
    {
        u64 tile_count_x;
        u64 tile_count_z;
        f32 tile_size_x;
        f32 tile_size_z;
        f32 texture_scale_x;
        f32 texture_scale_z;
        u64 chunk_count_x;
        u64 chunk_count_z;
        bool split_to_chunks = false;
        NormalCalculation normal_type = NormalCalculation::eNone;
        std::unique_ptr<Heightmap> heightmap = nullptr;
    };
    struct ocean_parameters_t
    {
        grid_parameters_t ocean_grid;
    };
    struct terrain_parameters_t
    {
        grid_parameters_t terrain_grid;
        std::optional<std::string> terrain_texture_1;
        std::optional<std::string> terrain_texture_2;
        std::optional<std::string> terrain_texture_3;
        std::optional<std::string> terrain_texture_4;
    };

    enum class LightMesh
    {
        eNoMesh,
        eQuad,
        eSphere
    };
    struct light_parameters_t
    {
        Light light_data;
        LightMesh mesh_type = LightMesh::eNoMesh;
        u32 mesh_size = 0u;
        std::optional<std::string> light_texture = std::nullopt;
    };



    class TextureManager;

	class EntityLoader
	{
        [[nodiscard]]
        std::vector<tecs::entity> LoadGrid(grid_parameters_t const& args);

	public:
        
        EntityLoader(tecs::registry& reg, ID3D11Device* device, TextureManager& texture_manager);

        [[maybe_unused]] std::vector<tecs::entity> LoadGLTFModel(model_parameters_t const&);

        [[maybe_unused]] tecs::entity LoadSkybox(skybox_parameters_t const&);

        [[maybe_unused]] tecs::entity LoadLight(light_parameters_t const&);

        [[maybe_unused]] std::vector<tecs::entity> LoadOcean(ocean_parameters_t const&);

        [[maybe_unused]] std::vector<tecs::entity> LoadTerrain(terrain_parameters_t const&);

        void LoadModelMesh(tecs::entity, model_parameters_t const&);
	private:
        tecs::registry& reg;
		TextureManager& texture_manager;
        ID3D11Device* device;
	};
}


