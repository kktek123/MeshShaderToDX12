//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************
//#include "../Shared.h"
#define THREADS_PER_WAVE 32
#define AS_GROUP_SIZE THREADS_PER_WAVE

#define CULL_FLAG 0x1
#define MESHLET_FLAG 0x2

#include "MeshletUtils.hlsli"

#define ROOT_SIG \
    "CBV(b0), \
     CBV(b1), \
     CBV(b2), \
     SRV(t0), SRV(t1), SRV(t2), SRV(t3), SRV(t4)"

struct Instance
{
    float4x4 World;
    float4x4 WorldInvTrans;
    float    Scale;
    uint     Flags;
};

struct Constants
{
    float4x4 World;
    float4x4 WorldView;
    float4x4 WorldViewProj;
    //uint     DrawMeshlets;

    //float4x4    View;
    //float4x4    ViewProj;
    float4      Planes[6];

    //float3      ViewPosition;
    //uint        HighlightedIndex;

    float3      CullViewPosition;
    //uint        SelectedIndex;

    uint        DrawMeshlets;
};


struct Vertex
{
    float3 Position;
    float3 Normal;
    float2 texcoord;
    float3 tangentModel;
};

struct VertexOut
{
    float4 PositionHS   : SV_Position;
    float3 PositionVS   : POSITION0;
    float3 Normal       : NORMAL0;
    uint   MeshletIndex : COLOR0;
};

struct Payload
{
    uint MeshletIndices[AS_GROUP_SIZE];
};

ConstantBuffer<Constants>   Constants           : register(b0);
ConstantBuffer<MeshInfo>    MeshInfo            : register(b1);
ConstantBuffer<Instance>    Instance            : register(b2);
StructuredBuffer<Vertex>    Vertices            : register(t0);
StructuredBuffer<Meshlet>   Meshlets            : register(t1);
ByteAddressBuffer           UniqueVertexIndices : register(t2);
StructuredBuffer<uint>      PrimitiveIndices    : register(t3);
StructuredBuffer<CullData>  MeshletCullData     : register(t4);


// Rotates a vector, v0, about an axis by some angle
float3 RotateVector(float3 v0, float3 axis, float angle)
{
    float cs = cos(angle);
    return cs * v0 + sin(angle) * cross(axis, v0) + (1 - cs) * dot(axis, v0) * axis;
}
