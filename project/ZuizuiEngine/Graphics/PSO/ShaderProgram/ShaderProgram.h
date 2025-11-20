#pragma once
#include <wrl.h>
#include <dxcapi.h>
#include <string>
#include <d3d12.h>

// HLSLのVSとPSをまとめて扱うクラス
class ShaderProgram {
public:
    ShaderProgram() = default;
    ~ShaderProgram() = default;

    // コンパイル関数
    bool CompileVS(
        const std::wstring& filepath,
        IDxcUtils* utils,
        IDxcCompiler3* compiler,
        IDxcIncludeHandler* includeHandler);

    bool CompilePS(
        const std::wstring& filepath,
        IDxcUtils* utils,
        IDxcCompiler3* compiler,
        IDxcIncludeHandler* includeHandler);

    // PSO用のバイトコード
    D3D12_SHADER_BYTECODE GetVS() const;
    D3D12_SHADER_BYTECODE GetPS() const;

    // コンパイル済みのBlobそのまま渡したいとき
    IDxcBlob* GetVSBlob() const { return vs_.Get(); }
    IDxcBlob* GetPSBlob() const { return ps_.Get(); }

private:
    Microsoft::WRL::ComPtr<IDxcBlob> vs_;
    Microsoft::WRL::ComPtr<IDxcBlob> ps_;
};
