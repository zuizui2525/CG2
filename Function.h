#pragma once
#include "Header.h"

// string->wstring変換
std::wstring ConvertString(const std::string& str);

// wstring->string変換
std::string ConvertString(const std::wstring& str);

// 出力ウィンドウに文字を出す
void Log(std::ostream& os, const std::string& message);

// ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// テクスチャを読み込む関数
DirectX::ScratchImage LoadTexture(const std::string& filePath);

// テクスチャリソースの生成
[[nodiscard]]
ID3D12Resource* CreateTextureResource(ID3D12Device* device, const DirectX::TexMetadata& metadata);

// バッファリソースを作成する関数
[[nodiscard]]
ID3D12Resource* CreateBufferResource(ID3D12Device* device, size_t sizeInBytes);

// テクスチャデータを転送する関数
ID3D12Resource* UploadTextureData(ID3D12Resource* texture, const DirectX::ScratchImage& mipImages, ID3D12Device* device, ID3D12GraphicsCommandList* commandList);

// ダンプファイルの生成
LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception);

// Dumpを出力する関数
IDxcBlob* CompileShader(
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
ID3D12DescriptorHeap* CreateDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);

// DepthStencilTextureの生成
ID3D12Resource* CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height);

// ディスクリプタヒープの先頭から指定したインデックスのCPUディスクリプタハンドルを取得する
D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);

// ディスクリプタヒープの先頭から指定したインデックスのGPUディスクリプタハンドルを取得する
D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index);