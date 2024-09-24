#pragma once
#include "VertexData.h"
using DirectX::SimpleMath::Vector2;
using DirectX::SimpleMath::Vector3;


class GeometryGenerator {
public:
public:
    static vector<MeshData> ReadFromFile(string basePath, string filename,
        bool revertNormals = false);

    //static auto ReadAnimationFromFile(string basePath, string filename,
    //    bool revertNormals = false)
    //    -> tuple<vector<MeshData>, AnimationData>;

    //static void Normalize(const Vector3 center, const float longestLength,
    //    vector<MeshData>& meshes, AnimationData& aniData);

    static MeshData MakeSquare(const float scale = 1.0f,
        const Vector2 texScale = Vector2(1.0f));
    static MeshData MakeSquareGrid(const int numSlices, const int numStacks,
        const float scale = 1.0f,
        const Vector2 texScale = Vector2(1.0f));
    static MeshData CreateGrid(float width, float depth, uint32_t m, uint32_t n);

    static MeshData MakeGrass();
    static MeshData MakeBox(const float scale = 1.0f);
    static MeshData MakeWireBox(const Vector3 center, const Vector3 extents);
    static MeshData MakeWireSphere(const Vector3 center, const float radius);
    static MeshData MakeCylinder(const float bottomRadius,
        const float topRadius, float height,
        int numSlices);
    static MeshData MakeSphere(const float radius, const int numSlices,
        const int numStacks,
        const Vector2 texScale = Vector2(1.0f));
    static MeshData MakeTetrahedron();
    static MeshData MakeIcosahedron();
    static MeshData SubdivideToSphere(const float radius, MeshData meshData);
};