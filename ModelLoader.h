#pragma once
#include <assimp\Importer.hpp>
#include <assimp\postprocess.h>
#include <assimp\scene.h>
#include <iostream>
#include <string>
#include <vector>

#include "VertexData.h"

class ModelLoader {
public:
    void Load(std::string basePath, std::string filename);

    void ProcessNode(aiNode* node, const aiScene* scene,
        DirectX::SimpleMath::Matrix tr);

    MeshData ProcessMesh(aiMesh* mesh, const aiScene* scene);

public:
    std::string basePath;
    std::vector<MeshData> meshes;
};
