#pragma once
#include <optional>
#include <array>
#include <vector>
#include "Components.h"
#include "Core/Paths.h"
#include "Math/ComputeNormals.h"
#include "Utilities/Heightmap.h"
#include "tecs/entity.h"

namespace adria
{
    
    enum class LightMesh
    {
        
        NoMesh,
        Quad,
        Cube
    };
	enum class FoliageMesh
	{
		SingleQuad,
		DoubleQuad,
		TripleQuad
	};
    enum class TreeType
    {
        Tree01,
        Tree02
    };

    struct LightParameters
    {
        Light light_data;
        LightMesh mesh_type = LightMesh::NoMesh;
        uint32 mesh_size = 0u;
        std::optional<std::string> light_texture = std::nullopt;
    };
	struct ModelParameters
	{
        std::string model_path = "";
        std::string textures_path = "";
        Matrix model_matrix = Matrix::Identity;
	};
    struct SkyboxParameters
    {
        std::optional<std::wstring> cubemap;
        std::array<std::string, 6> cubemap_textures;
    };
    struct GridParameters
    {
        uint64 tile_count_x;
        uint64 tile_count_z;
        float tile_size_x;
        float tile_size_z;
        float texture_scale_x;
        float texture_scale_z;
        uint64 chunk_count_x;
        uint64 chunk_count_z;
        bool split_to_chunks = false;
        ENormalCalculation normal_type = ENormalCalculation::None;
        std::unique_ptr<Heightmap> heightmap = nullptr;
        Vector3 grid_offset = Vector3(0,0,0);
    };
    struct OceanParameters
    {
        GridParameters ocean_grid;
    };
    struct FoliageParameters
    {
        std::pair<FoliageMesh, std::string> mesh_texture_pair;
        int32 foliage_count;
        float foliage_scale;
		Vector2 foliage_center;
		Vector2 foliage_extents;
        float foliage_height_start;
		float foliage_height_end;
		float foliage_slope_start;
    };
    struct TreeParameters
    {
        TreeType tree_type;
		int32 tree_count;
		float tree_scale;
		Vector2 tree_center;
		Vector2 tree_extents;
        float tree_height_start;
		float tree_height_end;
		float tree_slope_start;
    };
	struct TerrainTextureLayerParameters
	{
		float terrain_sand_start = -100.0f;
		float terrain_sand_end = 0.0f;
		float terrain_grass_start = 0.0f;
		float terrain_grass_end = 300.0f;
		float terrain_slope_grass_start = 0.92f;
		float terrain_slope_rocks_start = 0.85f;
		float terrain_rocks_start = 50.0f;
        float height_mix_zone = 50.0f;
        float slope_mix_zone = 0.025f;
	};
	struct TerrainParameters
	{
		GridParameters terrain_grid;
        TerrainTextureLayerParameters layer_params;
		std::string grass_texture;
		std::string base_texture;
		std::string rock_texture;
		std::string sand_texture;
        std::string layer_texture;
	};
    struct EmitterParameters
    {
        std::string name = "Emitter";
        float position[3] = { 50.0f, 10.0f, 0.0f};
        float velocity[3] = { 0.0f, 7.0f, 0.0f};
        float position_variance[3] = {4.0f, 0.0f, 4.0f};
        float velocity_variance = { 0.6f };
        float lifespan = 50.0f;
        float start_size = 22.0f;
        float end_size = 5.0f;
        float mass = 0.0003f;
        float particles_per_second = 100.0f;
        std::string texture_path = paths::TexturesDir() + "Particles/fire.png";
        bool blend = true;
        bool collisions = false;
        int32 collision_thickness = 40;
        bool sort = false;
    };
    struct DecalParameters
    {
        std::string name = "Decal";
        std::string albedo_texture_path = paths::TexturesDir() + "Decals/Decal_00_Albedo.tga";
        std::string normal_texture_path = paths::TexturesDir() + "Decals/Decal_00_Normal.png";
        float rotation = 0.0f;
        float size = 50.0f;
        DecalType decal_type = DecalType::Project_XY;
        bool modify_gbuffer_normals = false;
        Vector3 position;
        Vector3 normal;
    };

    namespace tecs
    {
        class registry;
    }
    class TextureManager;
	class GfxDevice;

	class ModelImporter
	{

	public:
        
        ModelImporter(tecs::registry& reg, GfxDevice* gfx);

        [[maybe_unused]] std::vector<tecs::entity> ImportModel_GLTF(ModelParameters const&);

        [[maybe_unused]] tecs::entity LoadSkybox(SkyboxParameters const&);
        [[maybe_unused]] tecs::entity LoadLight(LightParameters const&);
        [[maybe_unused]] std::vector<tecs::entity> LoadOcean(OceanParameters const&);
		[[maybe_unused]] std::vector<tecs::entity> LoadTerrain(TerrainParameters&);
		[[maybe_unused]] tecs::entity LoadFoliage(FoliageParameters const&);
        [[maybe_unused]] std::vector<tecs::entity> LoadTrees(TreeParameters const&);
        [[maybe_unused]] tecs::entity LoadEmitter(EmitterParameters const&);
        [[maybe_unused]] tecs::entity LoadDecal(DecalParameters const&);

	private:
        tecs::registry& reg;
		GfxDevice* gfx;

    private:

        [[nodiscard]] std::vector<tecs::entity> LoadObjMesh(std::string const& model_path, std::vector<std::string>* diffuse_textures_out = nullptr);
        [[nodiscard]] std::vector<tecs::entity> LoadGrid(GridParameters const& args, std::vector<TexturedNormalVertex>* vertices = nullptr);
	};
}


