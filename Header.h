#pragma once
#pragma warning(push)
#pragma warning(disable:4023)
#define _USE_MATH_DEFINES
#define DIRECTINPUT_VERSION 0x0800 // DirectInputのバージョン指定
#include <math.h>
#include <cstdint>
#include <string>
#include <format>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <chrono>
#include <cassert>
#include <vector>
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dbghelp.h>
#include <strsafe.h>
#include <dxgidebug.h>
#include <wrl.h>
#include <dxcapi.h>
#include <xaudio2.h>
#include <dinput.h>
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_dx12.h"
#include "externals/imgui/imgui_impl_win32.h"
#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"
#include "Struct.h"
#include "Matrix.h"
#include "DebugCamera.h"
#include "PipelineStateObject.h"
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "xaudio2.lib")
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#pragma warning(pop)