#pragma once
#include <Windows.h>
#include <dxcapi.h>
#include <wrl.h>
#include <string>
#include <ostream>

/// @brief Shaderコンパイルを担うクラス
class ShaderCompiler {
public:
	/// @brief HLSLファイルをコンパイルする
	/// @param os ログ出力先のストリーム
	/// @param filePath HLSLファイルへのパス
	/// @param profile コンパイルに使用するプロファイル (例: L"vs_6_0", L"ps_6_0")
	/// @param dxcUtils IDxcUtils
	/// @param dxcCompiler IDxcCompiler3
	/// @param includeHandler IDxcIncludeHandler
	/// @return コンパイルされたシェーダーバイナリ
	static Microsoft::WRL::ComPtr<IDxcBlob> Compile(
		std::ostream& os,
		const std::wstring& filePath,
		const wchar_t* profile,
		IDxcUtils* dxcUtils,
		IDxcCompiler3* dxcCompiler,
		IDxcIncludeHandler* includeHandler);
};
