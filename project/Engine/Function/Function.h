#pragma once

#define DIRECTINPUT_VERSION 0x0800 // 念のため（DirectXTex 内で参照される可能性）

#include <cstdint>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <cassert>

#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dbghelp.h>
#include <strsafe.h>
#include <dxgidebug.h>
#include <wrl.h>
#include <dxcapi.h>
#include <xaudio2.h>

#include "../../externals/DirectXTex/DirectXTex.h"
#include "../../externals/DirectXTex/d3dx12.h"
#include "../../externals/imgui/imgui_impl_dx12.h"
#include "../../externals/imgui/imgui_impl_win32.h"

#include "../Struct.h"
#include "../Matrix/Matrix.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")
#pragma comment(lib, "xaudio2.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// string->wstring変換
std::wstring ConvertString(const std::string& str);

// wstring->string変換
std::string ConvertString(const std::wstring& str);

// ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// テクスチャを読み込む関数
DirectX::ScratchImage LoadTexture(const std::string& filePath);

// テクスチャリソースの生成
[[nodiscard]]
Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata);

// バッファリソースを作成する関数
[[nodiscard]]
Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(ID3D12Device* device, size_t sizeInBytes);

// テクスチャデータを転送する関数
Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages, ID3D12Device* device, ID3D12GraphicsCommandList* commandList);

// ダンプファイルの生成
LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception);

// Dumpを出力する関数
Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(
	std::ostream& os,
	// CompilerするShaderファイルへのパス
	const std::wstring& filePath,
	// Compilerに使用するprofile
	const wchar_t* profile,
	// 初期化で生成したものを３つ
	IDxcUtils* dxcUtils,
	IDxcCompiler3* dxcCompiler,
	IDxcIncludeHandler* includeHandler);

// ディスクリプタヒープの生成
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

// DepthStencilTextureの生成
Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height);

// ディスクリプタヒープの先頭から指定したインデックスのCPUディスクリプタハンドルを取得する
D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);

// ディスクリプタヒープの先頭から指定したインデックスのGPUディスクリプタハンドルを取得する
D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);

// mtlファイルを読む関数
MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);

// Objファイルを読む関数
ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);

// 音声データの読み込み
SoundData SoundLoadWave(const char* filename);

// 音声データの解放
void SoundUnload(SoundData* soundData);

// 音声再生
void SoundPlayWave(IXAudio2* xAudio2, const SoundData& soundData);
