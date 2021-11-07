#include <unordered_map>
#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_NOEXCEPTION
#include "tiny_gltf.h"

#define TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_USE_MAPBOX_EARCUT
#include "tiny_obj_loader.h"

#include "EntityLoader.h"
#include "../Graphics/VertexTypes.h"
#include "../Graphics/TextureManager.h"
#include "../Logging/Logger.h"
#include "../Math/BoundingVolumeHelpers.h"
#include "../Math/ComputeTangentFrame.h"
#include "../Utilities/FilesUtil.h"
#include "../Utilities/Random.h"
#include "../Utilities/Heightmap.h"
#include "../Utilities/Image.h"

using namespace DirectX;
namespace adria 
{
    namespace
    {
		void GenerateTerrainLayerTexture(char const* texture_name, Terrain* terrain, terrain_texture_layer_parameters_t const& params)
		{
			auto [width, depth] = terrain->TileCounts();
			auto [tile_size_x, tile_size_z] = terrain->TileSizes();

			std::vector<BYTE> temp_layer_data(width * depth * 4);
			std::vector<BYTE> layer_data(width * depth * 4);
			for (u64 j = 0; j < depth; ++j)
			{
				for (u64 i = 0; i < width; ++i)
				{
					f32 x = i * tile_size_x;
					f32 z = j * tile_size_z;

					f32 height = terrain->HeightAt(x, z);
					f32 normal_y = terrain->NormalAt(x, z).y;

					if (height > params.terrain_rocks_start)
					{
						f32 mix_multiplier = std::max(
							(height - params.terrain_rocks_start) / params.height_mix_zone,
							1.0f
						);
						f32 rock_slope_multiplier = std::clamp((normal_y - params.terrain_slope_rocks_start) / params.slope_mix_zone, 0.0f, 1.0f);
						temp_layer_data[(j * width + i) * 4 + 0] = BYTE_MAX * mix_multiplier * rock_slope_multiplier;
					}

					if (height > params.terrain_sand_start && height <= params.terrain_sand_end)
					{
                        f32 mix_multiplier = std::min(
                            (height-params.terrain_sand_start) / params.height_mix_zone,
                            (params.terrain_sand_end - height) / params.height_mix_zone
						);
						temp_layer_data[(j * width + i) * 4 + 1] = BYTE_MAX * mix_multiplier;
					}

					if (height > params.terrain_grass_start && height <= params.terrain_grass_end)
					{
						f32 mix_multiplier = std::min(
							(height - params.terrain_grass_start) / params.height_mix_zone,
							(params.terrain_grass_end - height) / params.height_mix_zone
						);

						f32 grass_slope_multiplier = std::clamp((normal_y - params.terrain_slope_grass_start) / params.slope_mix_zone, 0.0f, 1.0f);
						temp_layer_data[(j * width + i) * 4 + 2] = BYTE_MAX * mix_multiplier * grass_slope_multiplier;
					}

					u32 sum = temp_layer_data[(j * width + i) * 4 + 0]
						+ temp_layer_data[(j * width + i) * 4 + 1] + temp_layer_data[(j * width + i) * 4 + 2] + 1;

					temp_layer_data[(j * width + i) * 4 + 0] = (BYTE)((temp_layer_data[(j * width + i) * 4 + 0] * 1.0f / sum) * BYTE_MAX);
					temp_layer_data[(j * width + i) * 4 + 1] = (BYTE)((temp_layer_data[(j * width + i) * 4 + 1] * 1.0f / sum) * BYTE_MAX);
					temp_layer_data[(j * width + i) * 4 + 2] = (BYTE)((temp_layer_data[(j * width + i) * 4 + 2] * 1.0f / sum) * BYTE_MAX);
				}
			}

			layer_data = temp_layer_data;

			for (size_t j = 2; j < depth - 2; ++j)
			{
				for (size_t i = 2; i < width - 2; ++i)
				{
					i32 n1 = 0, n2 = 0, n3 = 0, n4 = 0;
					for (i32 k = -2; k <= 2; ++k)
					{
						for (i32 l = -2; l <= 2; ++l)
						{
							n1 += (i32)temp_layer_data[((j + k) * width + i + l) * 4 + 0];
							n2 += (i32)temp_layer_data[((j + k) * width + i + l) * 4 + 1];
							n3 += (i32)temp_layer_data[((j + k) * width + i + l) * 4 + 2];
						}
					}
            
					layer_data[(j * width + i) * 4 + 0] = (BYTE)(n1 / 25);
					layer_data[(j * width + i) * 4 + 1] = (BYTE)(n2 / 25);
					layer_data[(j * width + i) * 4 + 2] = (BYTE)(n3 / 25);
				}
			}

			WriteImageTGA(texture_name, layer_data, width, depth);
		}
    }


    using namespace tecs;

    [[nodiscard]]
    std::vector<entity> EntityLoader::LoadGrid(grid_parameters_t const& params, std::vector<TexturedNormalVertex>* vertices_out)
    {
        if (params.heightmap)
        {
            ADRIA_ASSERT(params.heightmap->Depth() == params.tile_count_z + 1);
            ADRIA_ASSERT(params.heightmap->Width() == params.tile_count_x + 1);
        }

        std::vector<entity> chunks;
        std::vector<TexturedNormalVertex> vertices{};
        for (u64 j = 0; j <= params.tile_count_z; j++)
        {
            for (u64 i = 0; i <= params.tile_count_x; i++)
            {
                TexturedNormalVertex vertex{};

                f32 height = params.heightmap ? params.heightmap->HeightAt(i, j) : 0.0f;

                vertex.position = XMFLOAT3(i * params.tile_size_x + params.grid_offset.x, 
                    height + params.grid_offset.y, j * params.tile_size_z + params.grid_offset.z);
                vertex.uv = XMFLOAT2(i * 1.0f * params.texture_scale_x / (params.tile_count_x - 1), j * 1.0f * params.texture_scale_z / (params.tile_count_z - 1));
                vertex.normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
                vertices.push_back(vertex);
            }
        }

        if (!params.split_to_chunks)
        {
            std::vector<u32> indices{};
            u32 i1 = 0;
            u32 i2 = 1;
            u32 i3 = static_cast<u32>(i1 + params.tile_count_x + 1);
            u32 i4 = static_cast<u32>(i2 + params.tile_count_x + 1);
            for (u64 i = 0; i < params.tile_count_x * params.tile_count_z; ++i)
            {
                indices.push_back(i1);
                indices.push_back(i3);
                indices.push_back(i2);


                indices.push_back(i2);
                indices.push_back(i3);
                indices.push_back(i4);

                ++i1;
                ++i2;
                ++i3;
                ++i4;

                if (i1 % (params.tile_count_x + 1) == params.tile_count_x)
                {
                    ++i1;
                    ++i2;
                    ++i3;
                    ++i4;
                }
            }

            ComputeNormals(params.normal_type, vertices, indices);

            entity grid = reg.create();

            Mesh mesh{};
            mesh.indices_count = (u32)indices.size();
            mesh.vertex_buffer = std::make_shared<VertexBuffer>();
            mesh.index_buffer = std::make_shared<IndexBuffer>();
            mesh.vertex_buffer->Create(device, vertices);
            mesh.index_buffer->Create(device, indices);

            reg.emplace<Mesh>(grid, mesh);
            reg.emplace<Transform>(grid);

            BoundingBox aabb = AABBFromRange(vertices.begin(), vertices.end());
            reg.emplace<Visibility>(grid, aabb, true, true);

            chunks.push_back(grid);
        }
        else
        {
            std::vector<u32> indices{};
            for (size_t j = 0; j < params.tile_count_z; j += params.chunk_count_z)
            {
                for (size_t i = 0; i < params.tile_count_x; i += params.chunk_count_x)
                {
                    entity chunk = reg.create();

                    u32 const indices_count = static_cast<u32>(params.chunk_count_z * params.chunk_count_x * 3 * 2);
                    u32 const indices_offset = static_cast<u32>(indices.size());


                    std::vector<TexturedNormalVertex> chunk_vertices_aabb{};
                    for (size_t k = j; k < j + params.chunk_count_z; ++k)
                    {
                        for (size_t m = i; m < i + params.chunk_count_x; ++m)
                        {

                            u32 i1 = static_cast<u32>(k * (params.tile_count_x + 1) + m);
                            u32 i2 = static_cast<u32>(i1 + 1);
                            u32 i3 = static_cast<u32>((k + 1) * (params.tile_count_x + 1) + m);
                            u32 i4 = static_cast<u32>(i3 + 1);

                            indices.push_back(i1);
                            indices.push_back(i3);
                            indices.push_back(i2);

                            indices.push_back(i2);
                            indices.push_back(i3);
                            indices.push_back(i4);

                            chunk_vertices_aabb.push_back(vertices[i1]);
                            chunk_vertices_aabb.push_back(vertices[i2]);
                            chunk_vertices_aabb.push_back(vertices[i3]);
                            chunk_vertices_aabb.push_back(vertices[i4]);
                        }
                    }

                    Mesh mesh{};
                    mesh.indices_count = indices_count;
                    mesh.start_index_location = indices_offset;

                    reg.emplace<Mesh>(chunk, mesh);

                    reg.emplace<Transform>(chunk);

                    BoundingBox aabb = AABBFromRange(chunk_vertices_aabb.begin(), chunk_vertices_aabb.end());

                    reg.emplace<Visibility>(chunk, aabb, true, true);

                    chunks.push_back(chunk);

                }
            }

            ComputeNormals(params.normal_type, vertices, indices);

            std::shared_ptr<VertexBuffer> vb = std::make_shared<VertexBuffer>();
            std::shared_ptr<IndexBuffer> ib = std::make_shared<IndexBuffer>();
            vb->Create(device, vertices);
            ib->Create(device, indices);

            for (entity chunk : chunks)
            {
                auto& mesh = reg.get<Mesh>(chunk);

                mesh.vertex_buffer = vb;
                mesh.index_buffer = ib;
            }
        }

        if (vertices_out)
        {
            *vertices_out = vertices;
        }

        return chunks;
    }

	std::vector<entity> EntityLoader::LoadObjMesh(std::string const& model_path, std::vector<std::string>* diffuse_textures_out)
	{
		tinyobj::ObjReaderConfig reader_config{};
		tinyobj::ObjReader reader;
		std::string model_name = GetFilename(model_path);
		if (!reader.ParseFromFile(model_path, reader_config))
		{
			if (!reader.Error().empty())  Log::Error(reader.Error());
			return {};
		}
		if (!reader.Warning().empty())  Log::Warning(reader.Warning());

		tinyobj::attrib_t const& attrib = reader.GetAttrib();
		std::vector<tinyobj::shape_t> const& shapes = reader.GetShapes();
        std::vector<tinyobj::material_t> const& materials = reader.GetMaterials();
		
		
		std::vector<entity> entities{};

        std::vector<std::string> diffuse_textures;
		for (size_t s = 0; s < shapes.size(); s++)
		{
            std::vector<TexturedNormalVertex> vertices{};

			entity e = reg.create();
			entities.push_back(e);

			size_t index_offset = 0;
			for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
			{
				size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);

				for (size_t v = 0; v < fv; v++)
				{
					// access to vertex
					tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

					TexturedNormalVertex vertex{};
					tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
					tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
					tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

					vertex.position.x = vx;
					vertex.position.y = vy;
					vertex.position.z = vz;

					// Check if `normal_index` is zero or positive. negative = no normal data
					if (idx.normal_index >= 0)
					{
						tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
						tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
						tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index) + 2];

						vertex.normal.x = nx;
						vertex.normal.y = ny;
						vertex.normal.z = nz;
					}

					// Check if `texcoord_index` is zero or positive. negative = no texcoord data
					if (idx.texcoord_index >= 0)
					{
						tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
						tinyobj::real_t ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];

						vertex.uv.x = tx;
						vertex.uv.y = ty;
					}

					vertices.push_back(vertex);
				}

                index_offset += fv;
			}

			std::shared_ptr<VertexBuffer> vb = std::make_shared<VertexBuffer>();
			vb->Create(device, vertices);

			Mesh mesh_component{};
			mesh_component.base_vertex_location = 0;
			mesh_component.vertex_count = static_cast<u32>(vertices.size());
            mesh_component.vertex_buffer = vb;
			reg.emplace<Mesh>(e, mesh_component);

			reg.emplace<Tag>(e, model_name + " mesh" + std::to_string(as_integer(e)));

			if (diffuse_textures_out)
			{
				ADRIA_ASSERT(shapes[s].mesh.material_ids.size() > 0);
				int material_id = shapes[s].mesh.material_ids[0];
				ADRIA_ASSERT(material_id >= 0);
				tinyobj::material_t material = materials[material_id];
				diffuse_textures_out->push_back(material.diffuse_texname);
			}

		}

		Log::Info("OBJ Mesh" + model_path + " successfully loaded!");
		return entities;
	}


	EntityLoader::EntityLoader(registry& reg, ID3D11Device* device, TextureManager& texture_manager) : reg(reg),
        texture_manager(texture_manager), device(device)
    {}

    [[maybe_unused]]
    std::vector<entity> EntityLoader::LoadGLTFModel(model_parameters_t const& params)
    {
        tinygltf::TinyGLTF loader;
        tinygltf::Model model;
        std::string err;
        std::string warn;
        bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, params.model_path);

        std::string model_name = GetFilename(params.model_path);
        if (!warn.empty()) Log::Warning(warn.c_str());
        if (!err.empty()) Log::Error(err.c_str());
        if (!ret) Log::Error("Failed to load model " + model_name);
        
		std::vector<CompleteVertex> vertices{};
		std::vector<u32> indices{};
		std::vector<entity> entities{};

        tinygltf::Scene const& scene = model.scenes[model.defaultScene];
        for (size_t i = 0; i < scene.nodes.size(); ++i)
        {
            tinygltf::Node const& node = model.nodes[scene.nodes[i]]; //has children: model.nodes[node.children[i]] todo
            if (node.mesh < 0 || node.mesh >= model.meshes.size()) continue;

            tinygltf::Mesh const& node_mesh = model.meshes[node.mesh];
            for (size_t i = 0; i < node_mesh.primitives.size(); ++i)
            {
                tinygltf::Primitive primitive = node_mesh.primitives[i];
                tinygltf::Accessor const& index_accessor = model.accessors[primitive.indices];

                entity e = reg.create();
                entities.push_back(e);

                Mesh mesh_component{};
                mesh_component.indices_count = static_cast<u32>(index_accessor.count);
                mesh_component.start_index_location = static_cast<u32>(indices.size());
                mesh_component.base_vertex_location = static_cast<u32>(vertices.size());
                switch (primitive.mode)
                {
                case TINYGLTF_MODE_POINTS:
                    mesh_component.topology = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
                    break;
                case TINYGLTF_MODE_LINE:
                    mesh_component.topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
                    break;
                case TINYGLTF_MODE_LINE_STRIP:
                    mesh_component.topology = D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
                    break;
                case TINYGLTF_MODE_TRIANGLES:
                    mesh_component.topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
                    break;
                case TINYGLTF_MODE_TRIANGLE_STRIP:
                    mesh_component.topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
                    break;
                default:
                    assert(false);
                }
                reg.emplace<Mesh>(e, mesh_component);

                tinygltf::Accessor const& position_accessor = model.accessors[primitive.attributes["POSITION"]];
                tinygltf::BufferView const& position_buffer_view = model.bufferViews[position_accessor.bufferView];
                tinygltf::Buffer const& position_buffer = model.buffers[position_buffer_view.buffer];
                int const position_byte_stride = position_accessor.ByteStride(position_buffer_view);
                u8 const* positions = &position_buffer.data[position_buffer_view.byteOffset + position_accessor.byteOffset];

                tinygltf::Accessor const& texcoord_accessor = model.accessors[primitive.attributes["TEXCOORD_0"]];
                tinygltf::BufferView const& texcoord_buffer_view = model.bufferViews[texcoord_accessor.bufferView];
                tinygltf::Buffer const& texcoord_buffer = model.buffers[texcoord_buffer_view.buffer];
                int const texcoord_byte_stride = texcoord_accessor.ByteStride(texcoord_buffer_view);
                u8 const* texcoords = &texcoord_buffer.data[texcoord_buffer_view.byteOffset + texcoord_accessor.byteOffset];

                tinygltf::Accessor const& normal_accessor = model.accessors[primitive.attributes["NORMAL"]];
                tinygltf::BufferView const& normal_buffer_view = model.bufferViews[normal_accessor.bufferView];
                tinygltf::Buffer const& normal_buffer = model.buffers[normal_buffer_view.buffer];
                int const normal_byte_stride = normal_accessor.ByteStride(normal_buffer_view);
                u8 const* normals = &normal_buffer.data[normal_buffer_view.byteOffset + normal_accessor.byteOffset];

                tinygltf::Accessor const& tangent_accessor = model.accessors[primitive.attributes["TANGENT"]];
                tinygltf::BufferView const& tangent_buffer_view = model.bufferViews[tangent_accessor.bufferView];
                tinygltf::Buffer const& tangent_buffer = model.buffers[tangent_buffer_view.buffer];
                int const tangent_byte_stride = tangent_accessor.ByteStride(tangent_buffer_view);
                u8 const* tangents = &tangent_buffer.data[tangent_buffer_view.byteOffset + tangent_accessor.byteOffset];

                ADRIA_ASSERT(position_accessor.count == texcoord_accessor.count);
                ADRIA_ASSERT(position_accessor.count == normal_accessor.count);

                std::vector<XMFLOAT3> position_array(position_accessor.count), normal_array(normal_accessor.count),
                    tangent_array(position_accessor.count), bitangent_array(position_accessor.count);
                std::vector<XMFLOAT2> texcoord_array(texcoord_accessor.count);
                for (size_t i = 0; i < position_accessor.count; ++i)
                {
                    XMFLOAT3 position;
                    position.x = (reinterpret_cast<f32 const*>(positions + (i * position_byte_stride)))[0];
                    position.y = (reinterpret_cast<f32 const*>(positions + (i * position_byte_stride)))[1];
                    position.z = (reinterpret_cast<f32 const*>(positions + (i * position_byte_stride)))[2];
                    position_array[i] = position;

                    XMFLOAT2 texcoord;
                    texcoord.x = (reinterpret_cast<f32 const*>(texcoords + (i * texcoord_byte_stride)))[0];
                    texcoord.y = (reinterpret_cast<f32 const*>(texcoords + (i * texcoord_byte_stride)))[1];
                    texcoord.y = 1.0f - texcoord.y;
                    texcoord_array[i] = texcoord;

                    XMFLOAT3 normal;
                    normal.x = (reinterpret_cast<f32 const*>(normals + (i * normal_byte_stride)))[0];
                    normal.y = (reinterpret_cast<f32 const*>(normals + (i * normal_byte_stride)))[1];
                    normal.z = (reinterpret_cast<f32 const*>(normals + (i * normal_byte_stride)))[2];
                    normal_array[i] = normal;

                    XMFLOAT3 tangent{};
                    XMFLOAT3 bitangent{};
                    if (tangents)
                    {
                        tangent.x = (reinterpret_cast<f32 const*>(tangents + (i * tangent_byte_stride)))[0];
                        tangent.y = (reinterpret_cast<f32 const*>(tangents + (i * tangent_byte_stride)))[1];
                        tangent.z = (reinterpret_cast<f32 const*>(tangents + (i * tangent_byte_stride)))[2];
                        float tangent_w = (reinterpret_cast<f32 const*>(tangents + (i * tangent_byte_stride)))[3];

                        XMVECTOR _bitangent = XMVectorScale(XMVector3Cross(XMLoadFloat3(&normal), XMLoadFloat3(&tangent)), tangent_w);

                        XMStoreFloat3(&bitangent, XMVector3Normalize(_bitangent));
                    }

                    tangent_array[i] = tangent;
                    bitangent_array[i] = bitangent;
                }

                tinygltf::BufferView const& index_buffer_view = model.bufferViews[index_accessor.bufferView];
                tinygltf::Buffer const& index_buffer = model.buffers[index_buffer_view.buffer];
                int const index_byte_stride = index_accessor.ByteStride(index_buffer_view);
                u8 const* indexes = index_buffer.data.data() + index_buffer_view.byteOffset + index_accessor.byteOffset;

                indices.reserve(indices.size() + index_accessor.count);
                for (size_t i = 0; i < index_accessor.count; ++i)
                {
                    u32 index = -1;
                    switch (index_accessor.componentType)
                    {
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                        index = (u32)(reinterpret_cast<u16 const*>(indexes + (i * index_byte_stride)))[0];
                        break;
                    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                        index = (reinterpret_cast<u32 const*>(indexes + (i * index_byte_stride)))[0];
                        break;
                    default:
                        ADRIA_ASSERT(false);
                    }

                    indices.push_back(index);
                }

                if (!tangents) //need to generate tangents and bitangents
                {
                    ComputeTangentFrame(indices.data() - index_accessor.count, index_accessor.count,
                        position_array.data(), normal_array.data(), texcoord_array.data(), position_accessor.count,
                        tangent_array.data(), bitangent_array.data());
                }

                vertices.reserve(vertices.size() + position_accessor.count);
                for (size_t i = 0; i < position_accessor.count; ++i)
                {
                    vertices.push_back(CompleteVertex{
                            position_array[i],
                            texcoord_array[i],
                            normal_array[i],
                            tangent_array[i],
                            bitangent_array[i]
                        });
                }


                Material material{};
                tinygltf::Material gltf_material = model.materials[primitive.material];
                tinygltf::PbrMetallicRoughness pbr_metallic_roughness = gltf_material.pbrMetallicRoughness;

                if (pbr_metallic_roughness.baseColorTexture.index >= 0)
                {
                    tinygltf::Texture const& base_texture = model.textures[pbr_metallic_roughness.baseColorTexture.index];
                    tinygltf::Image const& base_image = model.images[base_texture.source];
                    std::string texbase = params.textures_path + base_image.uri;
                    material.albedo_texture = texture_manager.LoadTexture(texbase);
                    material.albedo_factor = (f32)pbr_metallic_roughness.baseColorFactor[0];
                }

                if (pbr_metallic_roughness.metallicRoughnessTexture.index >= 0)
                {
                    tinygltf::Texture const& metallic_roughness_texture = model.textures[pbr_metallic_roughness.metallicRoughnessTexture.index];
                    tinygltf::Image const& metallic_roughness_image = model.images[metallic_roughness_texture.source];
                    std::string texmetallicroughness = params.textures_path + metallic_roughness_image.uri;
                    material.metallic_roughness_texture = texture_manager.LoadTexture(texmetallicroughness);
                    material.metallic_factor = (f32)pbr_metallic_roughness.metallicFactor;
                    material.roughness_factor = (f32)pbr_metallic_roughness.roughnessFactor;
                }

                if (gltf_material.normalTexture.index >= 0)
                {
                    tinygltf::Texture const& normal_texture = model.textures[gltf_material.normalTexture.index];
                    tinygltf::Image const& normal_image = model.images[normal_texture.source];
                    std::string texnormal = params.textures_path + normal_image.uri;
                    material.normal_texture = texture_manager.LoadTexture(texnormal);
                }

                if (gltf_material.emissiveTexture.index >= 0)
                {
                    tinygltf::Texture const& emissive_texture = model.textures[gltf_material.emissiveTexture.index];
                    tinygltf::Image const& emissive_image = model.images[emissive_texture.source];
                    std::string texemissive = params.textures_path + emissive_image.uri;
                    material.emissive_texture = texture_manager.LoadTexture(texemissive);
                    material.emissive_factor = (f32)gltf_material.emissiveFactor[0];
                }
                material.shader = EShader::GBufferPBR;

                reg.emplace<Material>(e, material);

                XMMATRIX model = XMMatrixScaling(params.model_scale, params.model_scale, params.model_scale) * params.model_matrix;
                BoundingBox aabb = AABBFromRange(vertices.end() - position_accessor.count, vertices.end());
                aabb.Transform(aabb, model);

                reg.emplace<Visibility>(e, aabb, true, true);
                reg.emplace<Transform>(e, model, model);
                reg.emplace<Deferred>(e);
            }
        }

		std::shared_ptr<VertexBuffer> vb = std::make_shared<VertexBuffer>();
		std::shared_ptr<IndexBuffer> ib = std::make_shared<IndexBuffer>();
		vb->Create(device, vertices);
		ib->Create(device, indices);

		for (entity e : entities)
		{
			auto& mesh = reg.get<Mesh>(e);
			mesh.vertex_buffer = vb;
			mesh.index_buffer = ib;
			reg.emplace<Tag>(e, model_name + " mesh" + std::to_string(as_integer(e)));
		}

		Log::Info("Model" + params.model_path + " successfully loaded!");

        return entities;
	}

    [[maybe_unused]]
    entity EntityLoader::LoadSkybox(skybox_parameters_t const& params)
    {
        entity skybox = reg.create();

        Skybox sky{};
        sky.active = true;

        if (params.cubemap.has_value()) sky.cubemap_texture = texture_manager.LoadCubeMap(params.cubemap.value());
        else sky.cubemap_texture = texture_manager.LoadCubeMap(params.cubemap_textures);

        reg.emplace<Skybox>(skybox, sky);
        reg.emplace<Tag>(skybox, "Skybox");
        
        return skybox;
    }

    [[maybe_unused]] 
    entity EntityLoader::LoadLight(light_parameters_t const& params)
    {
        entity light = reg.create();

        if (params.light_data.type == ELightType::Directional)
            const_cast<light_parameters_t&>(params).light_data.position = XMVectorScale(-params.light_data.direction, 1e3);
  
        reg.emplace<Light>(light, params.light_data);

        if (params.mesh_type == ELightMesh::Quad)
        {
            u32 const size = params.mesh_size;
            std::vector<TexturedVertex> const vertices =
            {
                { {-0.5f * size, -0.5f * size, 0.0f}, {0.0f, 0.0f}},
                { { 0.5f * size, -0.5f * size, 0.0f}, {1.0f, 0.0f}},
                { { 0.5f * size,  0.5f * size, 0.0f}, {1.0f, 1.0f}},
                { {-0.5f * size,  0.5f * size, 0.0f}, {0.0f, 1.0f}}
            };
            std::vector<u16> const indices =
            {
                    0, 2, 1, 2, 0, 3
            };

            Mesh mesh{};
            mesh.vertex_buffer = std::make_shared<VertexBuffer>();
            mesh.index_buffer = std::make_shared<IndexBuffer>();
            mesh.vertex_buffer->Create(device, vertices);
            mesh.index_buffer->Create(device, indices);
            mesh.indices_count = static_cast<u32>(indices.size());

            reg.emplace<Mesh>(light, mesh);

            Material material{};
            XMStoreFloat3(&material.diffuse, params.light_data.color);

            if (params.light_texture.has_value())
                material.albedo_texture = texture_manager.LoadTexture(params.light_texture.value()); //
            else if(params.light_data.type == ELightType::Directional)
                material.albedo_texture = texture_manager.LoadTexture(L"Resources/Textures/sun.png");

            if (params.light_data.type == ELightType::Directional)
                material.shader = EShader::Sun;
            else if (material.albedo_texture != INVALID_TEXTURE_HANDLE)
                material.shader = EShader::Billboard;
            else GLOBAL_LOG_ERROR("Light with quad mesh needs diffuse texture!");

            reg.emplace<Material>(light, material); 
            
            BoundingBox aabb = AABBFromRange(vertices.begin(), vertices.end());
            auto translation_matrix = XMMatrixTranslationFromVector(params.light_data.position);
            aabb.Transform(aabb, XMMatrixTranslationFromVector(params.light_data.position));

            reg.emplace<Visibility>(light, aabb, true, false);
            reg.emplace<Transform>(light, translation_matrix, translation_matrix);
            //sun rendered in separate pass
            if (params.light_data.type != ELightType::Directional) reg.emplace<Forward>(light, true);
        }
        else if (params.mesh_type == ELightMesh::Cube)
        {
		    
           const SimpleVertex cube_vertices[8] = {
			   XMFLOAT3{ -1.0, -1.0,  1.0 },
			   XMFLOAT3{ 1.0, -1.0,  1.0 },
			   XMFLOAT3{ 1.0,  1.0,  1.0 },
			   XMFLOAT3{ -1.0,  1.0,  1.0 },
			   XMFLOAT3{ -1.0, -1.0, -1.0 },
			   XMFLOAT3{ 1.0, -1.0, -1.0 },
			   XMFLOAT3{ 1.0,  1.0, -1.0 },
			   XMFLOAT3{ -1.0,  1.0, -1.0 }
			};

			const uint16_t cube_indices[36] = {
				// front
				0, 1, 2,
				2, 3, 0,
				// right
				1, 5, 6,
				6, 2, 1,
				// back
				7, 6, 5,
				5, 4, 7,
				// left
				4, 0, 3,
				3, 7, 4,
				// bottom
				4, 5, 1,
				1, 0, 4,
				// top
				3, 2, 6,
				6, 7, 3
			};

			//Mesh skybox_mesh{};
			//skybox_mesh.vb = std::make_shared<VertexBuffer>();
			//skybox_mesh.ib = std::make_shared<IndexBuffer>();
			//skybox_mesh.vb->Create(device, cube_vertices, _countof(cube_vertices));
			//skybox_mesh.ib->Create(device, cube_indices, _countof(cube_indices));
			//skybox_mesh.indices_count = _countof(cube_indices);

			//reg.emplace<Mesh>(skybox, skybox_mesh);

            //load sphere mesh and mesh component
           //Mesh sphere_mesh{};
           //
           //
           //Material material{};
           //XMStoreFloat3(&material.diffuse, params.light_data.color);
           //
           //if (params.light_texture.has_value())
           //    material.diffuse_texture = texture_manager.LoadTexture(params.light_texture.value()); //
           //else if (params.light_data.type == LightType::eDirectional)
           //    material.diffuse_texture = texture_manager.LoadTexture(L"Resources/Textures/sun.png");
           //
           //if (params.light_data.type == LightType::eDirectional)
           //    material.shader = StandardShader::eSun;
           //else if (material.diffuse_texture == INVALID_TEXTURE_HANDLE)
           //    material.shader = StandardShader::eSolid;
           //else material.shader = StandardShader::eTexture;
           //
           //reg.emplace<Material>(light, material);
        }

        switch (params.light_data.type)
        {
        case ELightType::Directional:
            reg.emplace<Tag>(light, "Directional Light");
            break;
        case ELightType::Spot:
            reg.emplace<Tag>(light, "Spot Light");
            break;
        case ELightType::Point:
            reg.emplace<Tag>(light, "Point Light");
            break;
        }

        return light;
	}

    [[maybe_unused]]
    std::vector<entity> EntityLoader::LoadOcean(ocean_parameters_t const& params)
    {
        std::vector<entity> ocean_chunks = EntityLoader::LoadGrid(params.ocean_grid);

        Material ocean_material{};
        ocean_material.diffuse = XMFLOAT3(0.0123f, 0.3613f, 0.6867f); //0, 105, 148
        ocean_material.shader = EShader::Unknown; 

        Ocean ocean_component{};
        for (auto ocean_chunk : ocean_chunks)
        {
            reg.emplace<Material>(ocean_chunk, ocean_material);
            reg.emplace<Ocean>(ocean_chunk, ocean_component);
            reg.emplace<Tag>(ocean_chunk, "Ocean Chunk" + std::to_string(as_integer(ocean_chunk)));
        }

        return ocean_chunks;
	}

    [[maybe_unused]]
	std::vector<entity> EntityLoader::LoadTerrain(terrain_parameters_t& params)
	{
        std::vector<TexturedNormalVertex> vertices;
		std::vector<entity> terrain_chunks = LoadGrid(params.terrain_grid, &vertices);

        TerrainComponent::terrain = std::make_unique<Terrain>(vertices,
            params.terrain_grid.tile_size_x,
            params.terrain_grid.tile_size_z, 
            params.terrain_grid.tile_count_x,
            params.terrain_grid.tile_count_z);

        TerrainComponent::texture_scale = XMFLOAT2(params.terrain_grid.texture_scale_x,
            params.terrain_grid.texture_scale_z);

        GenerateTerrainLayerTexture(params.layer_texture.c_str(), TerrainComponent::terrain.get(), params.layer_params);

        TerrainComponent terrain_component{};
		terrain_component.grass_texture = texture_manager.LoadTexture(params.grass_texture);
		terrain_component.rock_texture = texture_manager.LoadTexture(params.rock_texture);
		terrain_component.base_texture = texture_manager.LoadTexture(params.base_texture);
		terrain_component.sand_texture = texture_manager.LoadTexture(params.sand_texture);
        terrain_component.layer_texture = texture_manager.LoadTexture(params.layer_texture);

		for (auto terrain_chunk : terrain_chunks)
		{
			reg.emplace<TerrainComponent>(terrain_chunk, terrain_component);
			reg.emplace<Tag>(terrain_chunk, "Terrain Chunk" + std::to_string(as_integer(terrain_chunk)));
		}

		return terrain_chunks;
	}

	[[maybe_unused]]
	entity EntityLoader::LoadFoliage(foliage_parameters_t const& params)
	{
		const f32 size = params.foliage_scale;
		
		struct FoliageInstance
		{
			XMFLOAT3 position;
            float rotation_y;
		};

		RealRandomGenerator<float> random_x(params.foliage_center.x - params.foliage_extents.x, params.foliage_center.x + params.foliage_extents.x);
		RealRandomGenerator<float> random_z(params.foliage_center.y - params.foliage_extents.y, params.foliage_center.y + params.foliage_extents.y);
        RealRandomGenerator<float> random_angle(0.0f, 2.0f * pi<f32>);

		std::vector<entity> foliages;

		switch (params.mesh_texture_pair.first)
		{
		case EFoliageMesh::SingleQuad:
			foliages = LoadObjMesh("Resources/Models/Foliage/foliagequad_single.obj");
			break;
		case EFoliageMesh::DoubleQuad:
			foliages = LoadObjMesh("Resources/Models/Foliage/foliagequad_double.obj");
			break;
		case EFoliageMesh::TripleQuad:
			foliages = LoadObjMesh("Resources/Models/Foliage/foliagequad_triple.obj");
			break;
		default:
			foliages = LoadObjMesh("Resources/Models/Foliage/foliagequad_single.obj");
			break;
		}
		ADRIA_ASSERT(foliages.size() == 1);
		entity foliage = foliages[0];

		std::vector<FoliageInstance> instance_data{};
		for (i32 i = 0; i < params.foliage_count; ++i)
		{
            static const u32 MAX_ITERATIONS = 5;
			XMFLOAT3 position{};
			XMFLOAT3 normal{};
			u32 iteration = 0;
			do
			{
                if (iteration > MAX_ITERATIONS) break;
				position.x = random_x();
				position.z = random_z();
				position.y = TerrainComponent::terrain ? TerrainComponent::terrain->HeightAt(position.x, position.z) - 0.5f : -0.5f;
				normal = TerrainComponent::terrain ? TerrainComponent::terrain->NormalAt(position.x, position.z) : XMFLOAT3(0.0f, 1.0f, 0.0f);

				++iteration;
			} while (position.y > params.foliage_height_end || position.y < params.foliage_height_start || normal.y < params.foliage_slope_start);

			if(iteration < MAX_ITERATIONS) instance_data.emplace_back(position, random_angle());
		}

		auto& mesh_component = reg.get<Mesh>(foliage);

		mesh_component.start_instance_location = 0;
		mesh_component.instance_buffer = std::make_shared<VertexBuffer>();
		mesh_component.instance_buffer->Create(device, instance_data);
		mesh_component.instance_count = (u32)instance_data.size();

		Material material{};
		material.albedo_texture = texture_manager.LoadTexture(params.mesh_texture_pair.second);
		material.albedo_factor = 1.0f;
		material.shader = EShader::GBuffer_Foliage;
		reg.emplace<Material>(foliage, material);
		reg.emplace<Foliage>(foliage);

		Transform transform{};
		transform.starting_transform = XMMatrixScaling(size, size, size);
		transform.current_transform = transform.starting_transform;
		reg.emplace<Transform>(foliage, transform);

		Visibility vis{};
		vis.skip_culling = true;
		reg.emplace<Visibility>(foliage, vis);

		return foliage;
	}

	std::vector<entity> EntityLoader::LoadTrees(tree_parameters_t const& params)
	{
		const f32 size = params.tree_scale;

		struct TreeInstance
		{
			XMFLOAT3 position;
			float rotation_y;
		};

		RealRandomGenerator<float> random_x(params.tree_center.x - params.tree_extents.x, params.tree_center.x + params.tree_extents.x);
		RealRandomGenerator<float> random_z(params.tree_center.y - params.tree_extents.y, params.tree_center.y + params.tree_extents.y);
		RealRandomGenerator<float> random_angle(0.0f, 2.0f * pi<f32>);

        std::vector<std::string> diffuse_textures{};
        std::vector<entity> trees;

        std::string texture_path;
        switch (params.tree_type)
        {
        case ETreeType::Tree01:
            trees = LoadObjMesh("Resources/Models/Trees/Tree01/tree02.obj", &diffuse_textures);
            texture_path = "Resources/Models/Trees/Tree01/";
            break;
        case ETreeType::Tree02:
        default:
            trees = LoadObjMesh("Resources/Models/Trees/Tree02/tree02.obj", &diffuse_textures);
            texture_path = "Resources/Models/Trees/Tree02/";
        }

        ADRIA_ASSERT(diffuse_textures.size() == trees.size());

		std::vector<TreeInstance> instance_data{};
		for (i32 i = 0; i < params.tree_count; ++i)
		{
			static const u32 MAX_ITERATIONS = 5;
			XMFLOAT3 position{};
			XMFLOAT3 normal{};
			u32 iteration = 0;
			do
			{
				if (iteration > MAX_ITERATIONS) break;
				position.x = random_x();
				position.z = random_z();
				position.y = TerrainComponent::terrain ? TerrainComponent::terrain->HeightAt(position.x, position.z) - 0.5f : -0.5f;
				normal = TerrainComponent::terrain ? TerrainComponent::terrain->NormalAt(position.x, position.z) : XMFLOAT3(0.0f, 1.0f, 0.0f);

				++iteration;
			} while (position.y > params.tree_height_end || position.y < params.tree_height_start || normal.y < params.tree_slope_start);

			if (iteration < MAX_ITERATIONS) instance_data.emplace_back(position, random_angle());
		}

        for (size_t i = 0; i < trees.size(); ++i)
        {
            auto tree = trees[i];
			auto& mesh_component = reg.get<Mesh>(tree);

			mesh_component.start_instance_location = 0;
			mesh_component.instance_buffer = std::make_shared<VertexBuffer>();
			mesh_component.instance_buffer->Create(device, instance_data);
			mesh_component.instance_count = (u32)instance_data.size();

			Material material{};
			material.albedo_texture = texture_manager.LoadTexture(texture_path + diffuse_textures[i]);
			material.albedo_factor = 1.0f;
			material.shader = EShader::GBuffer_Foliage;
			reg.emplace<Material>(tree, material);
			reg.emplace<Foliage>(tree);

			Transform transform{};
			transform.starting_transform = XMMatrixScaling(size, size, size);
			transform.current_transform = transform.starting_transform;
			reg.emplace<Transform>(tree, transform);

			Visibility vis{};
			vis.skip_culling = true;
			reg.emplace<Visibility>(tree, vis);
        }

		return trees;
	}

}