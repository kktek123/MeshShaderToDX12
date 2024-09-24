// pch.h: 미리 컴파일된 헤더 파일입니다.
// 아래 나열된 파일은 한 번만 컴파일되었으며, 향후 빌드에 대한 빌드 성능을 향상합니다.
// 코드 컴파일 및 여러 코드 검색 기능을 포함하여 IntelliSense 성능에도 영향을 미칩니다.
// 그러나 여기에 나열된 파일은 빌드 간 업데이트되는 경우 모두 다시 컴파일됩니다.
// 여기에 자주 업데이트할 파일을 추가하지 마세요. 그러면 성능이 저하됩니다.

#define IDS_APP_TITLE			103

#define IDR_MAINFRAME			128
#define IDD_MY01CREATEDEVICE_DIALOG	102
#define IDD_ABOUTBOX			103
#define IDM_ABOUT				104
#define IDM_EXIT				105
#define IDI_MY01CREATEDEVICE			107
#define IDI_SMALL				108
#define IDC_MY01CREATEDEVICE			109
#define IDC_MYICON				2
#ifndef IDC_STATIC
#define IDC_STATIC				-1
#endif
// Next default values for new objects
//
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS

#define _APS_NO_MFC					130
#define _APS_NEXT_RESOURCE_VALUE	129
#define _APS_NEXT_COMMAND_VALUE		32771
#define _APS_NEXT_CONTROL_VALUE		1000
#define _APS_NEXT_SYMED_VALUE		110
#endif
#endif

//#ifndef PCH_H
//#define PCH_H

// 여기에 미리 컴파일하려는 헤더 추가

#include <initguid.h>
#include <iostream>

#include <Windows.h>
#include <assert.h>
#include <fstream>
#include <wrl.h>
#include <shellapi.h>

//STL
#include <string>
#include <vector>
#include <list>
#include <map>
#include <unordered_map>
#include <functional>
#include <iterator>
#include <thread>
#include <mutex>
using namespace std;

#include <d3d12.h>
#include <DirectXTex.h>
//#pragma comment(lib, "DirectXTex.lib")
#include <dxgi1_4.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <dwrite.h>
#include <d2d1_3.h>
#include <dwrite_3.h>
#include <d3d11on12.h>
//#include <d3dx12.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <SimpleMath.h>
#include <DirectXPackedVector.h>
#include "d3dx12.h"
#include <wrl/client.h> // ComPtr


using namespace DirectX;
using namespace DirectX::SimpleMath;
using namespace DirectX::PackedVector;
#define SafeRelease(p){ if(p){ (p)->Release(); (p) = NULL; } }
#define SafeDelete(p){ if(p){ delete (p); (p) = NULL; } }
#define SafeDeleteArray(p){ if(p){ delete [] (p); (p) = NULL; } }

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    if(FAILED(hr__)) { __debugbreak(); } \
}
#endif

#ifndef ThrowIfFalse
#define ThrowIfFalse(x)                                              \
{                                                                     \
    bool value = (x);                                               \
    if(!value) { __debugbreak(); } \
}
#endif

//#endif //PCH_H
