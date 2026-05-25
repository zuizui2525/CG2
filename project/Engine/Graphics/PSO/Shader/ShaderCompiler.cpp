#include "Engine/Graphics/PSO/Shader/ShaderCompiler.h"
#include "Engine/Base/Log/Log.h"
#include "Engine/Base/Utils/StringUtility.h"
#include <cassert>
#include <format>
#include <chrono>

Microsoft::WRL::ComPtr<IDxcBlob> ShaderCompiler::Compile(
	std::ostream& os,
	const std::wstring& filePath,
	const wchar_t* profile,
	IDxcUtils* dxcUtils,
	IDxcCompiler3* dxcCompiler,
	IDxcIncludeHandler* includeHandler) {

	Log::Write(std::format(L" ├─ 【シェーダーコンパイル開始】 ファイル:「{}」 | プロファイル: 「{}」", filePath, profile));
	auto startTime = std::chrono::steady_clock::now();

	// 1.hlslファイルを読む
	Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderSource = nullptr;
	HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
	assert(SUCCEEDED(hr));
 
	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8;
 
	// 2.Compileする
	LPCWSTR arguments[] = {
		filePath.c_str(),
		L"-E", L"main",
		L"-T", profile,
		L"-Zi", L"-Qembed_debug",
		L"-Od",
		L"-Zpr",
	};
 
	Microsoft::WRL::ComPtr<IDxcResult> shaderResult = nullptr;
	hr = dxcCompiler->Compile(
		&shaderSourceBuffer,
		arguments,
		_countof(arguments),
		includeHandler,
		IID_PPV_ARGS(&shaderResult)
	);
	assert(SUCCEEDED(hr));
 
	// 3.警告・エラーが出ていないか確認する
	Microsoft::WRL::ComPtr<IDxcBlobUtf8> shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
		std::string errStr = shaderError->GetStringPointer();
		Log::Write(os, std::format(" │   ├─ [★警告/エラー] {}", errStr));
		Log::Write(os, std::string("Shader Compile Warning/Error: ") + errStr);
		assert(false);
	}
 
	// 4.Compile結果を受け取って返す
	Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));
 
	auto endTime = std::chrono::steady_clock::now();
	float elapsed = std::chrono::duration<float>(endTime - startTime).count();

	Log::Write(std::format(L" └─ 【シェーダーコンパイル完了】 所要時間: {:.4f}秒", elapsed));

	return shaderBlob;
}
