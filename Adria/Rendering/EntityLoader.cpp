
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/pbrmaterial.h"

#include "EntityLoader.h"
#include "../Graphics/VertexTypes.h"
#include "../Graphics/TextureManager.h"
#include "../Logging/Logger.h"
#include "../Math/BoundingVolumeHelpers.h"
#include "../Utilities/FilesUtil.h"
#include "../Utilities/Heightmap.h"

using namespace DirectX;

namespace adria 
{
    using namespace tecs;

    [[nodiscard]]
    std::vector<entity> EntityLoader::LoadGrid(grid_parameters_t const& params)
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

                vertex.position = XMFLOAT3(i * params.tile_size_x, height, j * params.tile_size_z);
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
            mesh.vb = std::make_shared<VertexBuffer>();
            mesh.ib = std::make_shared<IndexBuffer>();
            mesh.vb->Create(device, vertices);
            mesh.ib->Create(device, indices);

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

                mesh.vb = vb;
                mesh.ib = ib;
            }

        }

        return chunks;

    }

    EntityLoader::EntityLoader(registry& reg,ID3D11Device* device, TextureManager& texture_manager) : reg(reg),
        texture_manager(texture_manager), device(device)
    {}

    [[maybe_unused]]
    std::vector<entity> EntityLoader::LoadGLTFModel(model_parameters_t const& params)
    {
        Assimp::Importer importer;

       auto importer_flags =
            aiProcess_CalcTangentSpace |
            aiProcess_GenSmoothNormals |
            aiProcess_JoinIdenticalVertices |
            aiProcess_OptimizeMeshes |
            aiProcess_ImproveCacheLocality |
            aiProcess_RemoveRedundantMaterials |
            aiProcess_LimitBoneWeights |
            aiProcess_SplitLargeMeshes |
            aiProcess_Triangulate |
            aiProcess_GenUVCoords |
            aiProcess_SortByPType |
            aiProcess_FindDegenerates |
            aiProcess_FindInvalidData |
            aiProcess_FindInstances |
            aiProcess_ValidateDataStructure |
            aiProcess_Debone;

        
        aiScene const* scene = importer.ReadFile(params.model_path, importer_flags);
        if (scene == nullptr)
        {
            GLOBAL_LOG_ERROR("Assimp Model Loading Failed for file " + params.model_path + "!");
            return {};
        }

        auto model_name = GetFilename(params.model_path);

        std::vector<CompleteVertex> vertices{};
        std::vector<u32> indices{};
        std::vector<entity> entities{};
        for (u32 mesh_index = 0; mesh_index < scene->mNumMeshes; mesh_index++)
        {
            entity e = reg.create();
            entities.push_back(e);

            const aiMesh* mesh = scene->mMeshes[mesh_index];

            Mesh mesh_component{};
            mesh_component.indices_count = static_cast<u32>(mesh->mNumFaces * 3);
            mesh_component.start_index_location = static_cast<u32>(indices.size());
            mesh_component.vertex_offset = static_cast<u32>(vertices.size());
            reg.emplace<Mesh>(e, mesh_component);

            vertices.reserve(vertices.size() + mesh->mNumVertices);

            for (u32 i = 0; i < mesh->mNumVertices; ++i)
            {
                vertices.push_back(
                    {
                        reinterpret_cast<XMFLOAT3&>(mesh->mVertices[i]),
                        reinterpret_cast<XMFLOAT2&>(mesh->mTextureCoords[0][i]),
                        reinterpret_cast<XMFLOAT3&>(mesh->mNormals[i]),
                        reinterpret_cast<XMFLOAT3&>(mesh->mTangents[i]),
                        reinterpret_cast<XMFLOAT3&>(mesh->mBitangents[i])
                    }
                );
            }


            
            
            indices.reserve(indices.size() + mesh->mNumFaces * 3);

            for (u32 i = 0; i < mesh->mNumFaces; ++i)
            {
                indices.push_back(mesh->mFaces[i].mIndices[0]);
                indices.push_back(mesh->mFaces[i].mIndices[1]);
                indices.push_back(mesh->mFaces[i].mIndices[2]);
            }

            const aiMaterial* srcMat = scene->mMaterials[mesh->mMaterialIndex];


            Material material{};

            float albedo_factor = 1.0f, metallic_factor = 1.0f, roughness_factor = 1.0f, emissive_factor = 1.0f;


            //if (srcMat->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_FACTOR, albedo_factor) == AI_SUCCESS)
            //{
            //    material.albedo_factor = albedo_factor;
            //}

            if (srcMat->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR, metallic_factor) == AI_SUCCESS)
            {
                material.metallic_factor = metallic_factor;
            }

            if (srcMat->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, roughness_factor) == AI_SUCCESS)
            {
                material.roughness_factor = roughness_factor;
            }
            //if (srcMat->Get(AI_MATKEY_COLOR_EMISSIVE, emissive_factor) == AI_SUCCESS)
            //{
            //    material.emissive_factor = emissive_factor;
            //}

            aiString texAlbedoPath;
            aiString texRoughnessMetallicPath;
            aiString texNormalPath;
            aiString texEmissivePath;


            if (AI_SUCCESS == srcMat->Get(AI_MATKEY_TEXTURE(aiTextureType_DIFFUSE, 1), texAlbedoPath))
            {
                std::string texalbedo = params.textures_path + texAlbedoPath.C_Str();

                material.albedo_texture = texture_manager.LoadTexture(texalbedo);
            }

            if (AI_SUCCESS == srcMat->Get(AI_MATKEY_TEXTURE(aiTextureType_UNKNOWN, 0), texRoughnessMetallicPath))
            {
                std::string texmetallicroughness = params.textures_path + texRoughnessMetallicPath.C_Str();

                material.metallic_roughness_texture = texture_manager.LoadTexture(texmetallicroughness);
            }

            if (AI_SUCCESS == srcMat->Get(AI_MATKEY_TEXTURE(aiTextureType_NORMALS, 0), texNormalPath))
            {

                std::string texnorm = params.textures_path + texNormalPath.C_Str();

                material.normal_texture = texture_manager.LoadTexture(texnorm);
            }

            if (AI_SUCCESS == srcMat->Get(AI_MATKEY_TEXTURE(aiTextureType_EMISSIVE, 0), texEmissivePath))
            {
                std::string texemissive = params.textures_path + texEmissivePath.C_Str();

                material.emissive_texture = texture_manager.LoadTexture(texemissive);

            }

            material.shader = (material.emissive_texture == INVALID_TEXTURE_HANDLE ? 
                StandardShader::eGbufferPBR : StandardShader::eGbufferPBR_Emissive);

            reg.emplace<Material>(e, material);

            //reg.AddComponent<DeferredTag>(e);
            BoundingBox aabb = AABBFromRange(vertices.end() - mesh->mNumVertices, vertices.end());
            aabb.Transform(aabb, params.model);

            reg.emplace<Visibility>(e, aabb, true, true);
            reg.emplace<Transform>(e, params.model, params.model);
            reg.emplace<Deferred>(e);

        }


        std::shared_ptr<VertexBuffer> vb = std::make_shared<VertexBuffer>();
        std::shared_ptr<IndexBuffer> ib  = std::make_shared<IndexBuffer>();
        vb->Create(device, vertices);
        ib->Create(device, indices);

        for (entity e : entities)
        {
            auto& mesh = reg.get<Mesh>(e);
            mesh.vb = vb;
            mesh.ib = ib;
            mesh.vertices = vertices;
            reg.emplace<Tag>(e, model_name + " mesh" + std::to_string(as_integer(e)));

        }

        Log::Info("Model" + params.model_path + " successfully loaded!");
       
        return entities;
    }

    [[maybe_unused]]
    entity EntityLoader::LoadSkybox(skybox_parameters_t const& params)
    {
        entity skybox = reg.create();

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

        Mesh skybox_mesh{};
        skybox_mesh.vb = std::make_shared<VertexBuffer>();
        skybox_mesh.ib = std::make_shared<IndexBuffer>();
        skybox_mesh.vb->Create(device, cube_vertices, _countof(cube_vertices));
        skybox_mesh.ib->Create(device, cube_indices, _countof(cube_indices));
        skybox_mesh.indices_count = _countof(cube_indices);

        reg.emplace<Mesh>(skybox, skybox_mesh);

        reg.emplace<Transform>(skybox, Transform{});

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

        if (params.light_data.type == LightType::eDirectional)
            const_cast<light_parameters_t&>(params).light_data.position = XMVectorScale(-params.light_data.direction, 1e3);
  
        reg.emplace<Light>(light, params.light_data);

        if (params.mesh_type == LightMesh::eQuad)
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
            mesh.vb = std::make_shared<VertexBuffer>();
            mesh.ib = std::make_shared<IndexBuffer>();
            mesh.vb->Create(device, vertices);
            mesh.ib->Create(device, indices);
            mesh.indices_count = static_cast<u32>(indices.size());

            reg.emplace<Mesh>(light, mesh);

            Material material{};
            XMStoreFloat3(&material.diffuse, params.light_data.color);

            if (params.light_texture.has_value())
                material.diffuse_texture = texture_manager.LoadTexture(params.light_texture.value()); //
            else if(params.light_data.type == LightType::eDirectional)
                material.diffuse_texture = texture_manager.LoadTexture(L"Resources/Textures/sun.png");

            if (params.light_data.type == LightType::eDirectional)
                material.shader = StandardShader::eSun;
            else if (material.diffuse_texture != INVALID_TEXTURE_HANDLE)
                material.shader = StandardShader::eBillboard;
            else GLOBAL_LOG_ERROR("Light with quad mesh needs diffuse texture!");

            reg.emplace<Material>(light, material); 
            
            BoundingBox aabb = AABBFromRange(vertices.begin(), vertices.end());
            auto translation_matrix = XMMatrixTranslationFromVector(params.light_data.position);
            aabb.Transform(aabb, XMMatrixTranslationFromVector(params.light_data.position));

            reg.emplace<Visibility>(light, aabb, true, false);
            reg.emplace<Transform>(light, translation_matrix, translation_matrix);
            reg.emplace<Forward>(light, true);
        }
        else if (params.mesh_type == LightMesh::eSphere)
        {
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
        case LightType::eDirectional:
            reg.emplace<Tag>(light, "Directional Light");
            break;
        case LightType::eSpot:
            reg.emplace<Tag>(light, "Spot Light");
            break;
        case LightType::ePoint:
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
        ocean_material.specular = XMFLOAT3(0.8f, 0.8f, 0.8f);
        ocean_material.shader = StandardShader::eUnknown; //not necessary

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
    std::vector<tecs::entity> EntityLoader::LoadTerrain(terrain_parameters_t const& params)
    {
        std::vector<entity> terrain_chunks = EntityLoader::LoadGrid(params.terrain_grid);


        Terrain terrain_component{};
        //Terrain::heightmap = params.terrain_grid.heightmap;
        Terrain::grass_texture = texture_manager.LoadTexture(params.grass_texture);
        Terrain::rock_texture = texture_manager.LoadTexture(params.rock_texture);
        Terrain::snow_texture = texture_manager.LoadTexture(params.snow_texture);
        Terrain::sand_texture = texture_manager.LoadTexture(params.sand_texture);

        for (auto terrain_chunk : terrain_chunks)
        {
            reg.emplace<Terrain>(terrain_chunk, terrain_component);
            reg.emplace<Tag>(terrain_chunk, "Terrain Chunk" + std::to_string(as_integer(terrain_chunk)));
        }

        return terrain_chunks;
    }

    void EntityLoader::LoadModelMesh(tecs::entity e, model_parameters_t const& params)
    {
        
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(params.model_path,
            aiProcess_CalcTangentSpace |
            aiProcess_JoinIdenticalVertices |
            aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_SplitLargeMeshes |
            aiProcess_RemoveRedundantMaterials |
            aiProcess_GenUVCoords |
            aiProcess_TransformUVCoords |
            aiProcess_FlipUVs |
            aiProcess_OptimizeMeshes |
            aiProcess_OptimizeGraph
        );

        if (scene == nullptr)
        {
            Log::Error(std::string("Unsuccessful Import: ") + importer.GetErrorString() + "\n");
            return;
        }
            

        std::vector<CompleteVertex> vertices;
        std::vector<u32> indices;

        if(scene->mNumMeshes > 1) Log::Warning("LoadModelMesh excepts only one mesh but there are more than one! \n");


        for (u32 meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++)
        {

            const aiMesh* mesh = scene->mMeshes[meshIndex];

            Mesh mesh_component{};
            mesh_component.indices_count = static_cast<u32>(mesh->mNumFaces * 3);
            mesh_component.start_index_location = static_cast<u32>(indices.size());
            mesh_component.vertex_offset = static_cast<u32>(vertices.size());

            reg.emplace<Mesh>(e, mesh_component);

            vertices.reserve(vertices.size() + mesh->mNumVertices);


            
            for (u32 i = 0; i < mesh->mNumVertices; ++i)
            {
                vertices.push_back(
                    {
                        reinterpret_cast<DirectX::XMFLOAT3&>(mesh->mVertices[i]),
                        mesh->HasTextureCoords(0) ? reinterpret_cast<DirectX::XMFLOAT2 const&>(mesh->mTextureCoords[0][i]) : DirectX::XMFLOAT2{},
                        reinterpret_cast<DirectX::XMFLOAT3&>(mesh->mNormals[i]),
                        mesh->HasTangentsAndBitangents() ? reinterpret_cast<XMFLOAT3 const&>(mesh->mTangents[i]) : DirectX::XMFLOAT3{},
                        mesh->HasTangentsAndBitangents() ? reinterpret_cast<XMFLOAT3 const&>(mesh->mBitangents[i]) : DirectX::XMFLOAT3{},
                    }
                );
            }

            indices.reserve(indices.size() + mesh->mNumFaces * 3);

            for (u32 i = 0; i < mesh->mNumFaces; ++i)
            {
                indices.push_back(mesh->mFaces[i].mIndices[0]);
                indices.push_back(mesh->mFaces[i].mIndices[1]);
                indices.push_back(mesh->mFaces[i].mIndices[2]);
            }

        }

        std::shared_ptr<VertexBuffer> vb = std::make_shared<VertexBuffer>();
        std::shared_ptr<IndexBuffer> ib = std::make_shared<IndexBuffer>();
        vb->Create(device, vertices);
        ib->Create(device, indices);

        auto& mesh = reg.get<Mesh>(e);

        mesh.vb = vb;
        mesh.ib = ib;
        mesh.vertices = vertices;

        auto model_name = GetFilename(params.model_path);

        std::string name = model_name + std::to_string(as_integer(e));

        auto tag = reg.get_if<Tag>(e);
        if (tag)
            tag->name = name;
        else reg.emplace<Tag>(e, name);


    }

}