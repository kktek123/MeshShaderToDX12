#include "pch.h"

#define THREADS_PER_WAVE 32
#define AS_GROUP_SIZE THREADS_PER_WAVE

#define CULL_FLAG 0x1
#define MESHLET_FLAG 0x2

#ifdef __cplusplus
using float4x4 = DirectX::XMFLOAT4X4;
using float4 = DirectX::XMFLOAT4;
using float3 = DirectX::XMFLOAT3;
using float2 = DirectX::XMFLOAT2;
using uint = uint32_t;
#endif

#ifdef __cplusplus
_declspec(align(256u))
#endif
struct Instance
{
    float4x4 World;
    float4x4 WorldInvTrans;
    float    Scale;
    uint     Flags;
};

#ifdef __cplusplus
_declspec(align(256u))
#endif
struct Constants
{
    XMFLOAT4X4 World;
    XMFLOAT4X4 WorldView;
    XMFLOAT4X4 WorldViewProj;

    //float4x4    View;
    //float4x4    ViewProj;
    float4      Planes[6];

    //float3      ViewPosition;
    //uint        HighlightedIndex;

    float3      CullViewPosition;
    //uint        SelectedIndex;

    uint        DrawMeshlets;
};
