#pragma once
#include "pch.h"


struct BasicVertex
{
    BasicVertex() {}
    BasicVertex(
        const DirectX::XMFLOAT3& p,
        const DirectX::XMFLOAT3& n,
        const DirectX::XMFLOAT3& t,
        const DirectX::XMFLOAT2& uv) :
        position(p),
        normal(n),
        Tangent(t),
        texCoord(uv) {}
    BasicVertex(
        float px, float py, float pz,
        float nx, float ny, float nz,
        float tx, float ty, float tz,
        float u, float v) :
        position(px, py, pz),
        normal(nx, ny, nz),
        Tangent(tx, ty, tz),
        texCoord(u, v) {}

    XMFLOAT3 position;
    XMFLOAT3 normal;
    XMFLOAT2 texCoord;
    XMFLOAT3 Tangent;
};

using DirectX::SimpleMath::Vector2;
using DirectX::SimpleMath::Vector3;

struct Vertex {
    Vector3 position;
    Vector3 normalModel;
    Vector2 texcoord;
    Vector3 tangentModel;
};

struct MeshData {
    std::vector<Vertex> vertices;
    //vector<SkinnedVertex> skinnedVertices;
    vector<uint32_t> indices;
    string albedoTextureFilename;
    string emissiveTextureFilename;
    string normalTextureFilename;
    string heightTextureFilename;
    string aoTextureFilename; // Ambient Occlusion
    string metallicTextureFilename;
    string roughnessTextureFilename;
    string opacityTextureFilename;

    std::vector<uint16_t>& GetIndices16()
    {
        if (mIndices16.empty())
        {
            mIndices16.resize(indices.size());
            for (size_t i = 0; i < indices.size(); ++i)
                mIndices16[i] = static_cast<uint16_t>(indices[i]);
        }

        return mIndices16;
    }

private:
    vector<uint16_t> mIndices16;
};