#include "Function.h"
#include "Log.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "../../Base/Utils/StringUtility.h"

Log logger;


// ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
#ifdef _USEIMGUI
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
		return true;
	}
#endif

	// メッセージに応じてゲーム固有の処理を行う
	switch (msg) {
		// ウィンドウが廃棄された
	case WM_DESTROY:
		// OSに対して、アプリの終了を伝える
		PostQuitMessage(0);
		return 0;
	}

	// 標準のメッセージ処理を行う
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

// バッファリソースを作成する関数
[[nodiscard]]
Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(ID3D12Device* device, size_t sizeInBytes) {
	// 頂点リソース用のヒープの設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties{};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; // Uploadheapを使う
	// 頂点リソースの設定
	D3D12_RESOURCE_DESC vertexResourceDesc{};
	// バッファリソース。テクスチャの場合はまた別の設定をする
	vertexResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	vertexResourceDesc.Width = sizeInBytes; //リソースのサイズ。
	// バッファの場合はこれらは１にする決まり
	vertexResourceDesc.Height = 1;
	vertexResourceDesc.DepthOrArraySize = 1;
	vertexResourceDesc.MipLevels = 1;
	vertexResourceDesc.SampleDesc.Count = 1;
	// バッファの場合はこれにする決まり
	vertexResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	// 実際に頂点リソースを作る
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr; // ResourceをComPtrで宣言
	HRESULT hr = device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE,
		&vertexResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
		IID_PPV_ARGS(&resource)); // ComPtrの&演算子オーバーロードを利用
	assert(SUCCEEDED(hr));
	return resource; // ComPtr<ID3D12Resource>を返す
}

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
	IDxcIncludeHandler* includeHandler) {
	// 1.hlslファイルを読む
	// これからシェーダーをコンパイルする旨をログに出す
	logger.Write(os, ConvertString(std::format(L"Begin CompileShader, path:{}, profile:{}\n", filePath, profile)));
	// hlslファイルを読む
	Microsoft::WRL::ComPtr<IDxcBlobEncoding> shaderSource = nullptr;
	HRESULT hr = dxcUtils->LoadFile(filePath.c_str(), nullptr, &shaderSource);
	// 読めなかったら止める
	assert(SUCCEEDED(hr));
	// 読み込んだファイルの内容を設定する
	DxcBuffer shaderSourceBuffer;
	shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
	shaderSourceBuffer.Size = shaderSource->GetBufferSize();
	shaderSourceBuffer.Encoding = DXC_CP_UTF8; // UTF8の文字コードであることを通知

	// 2.Compileする
	LPCWSTR arguments[] = {
		filePath.c_str(), // コンパイル対象のhlslファイル名
		L"-E",L"main", // エントリーポイントの指定。基本的にmain以外にはしない
		L"-T",profile, // ShaderProfileの設定
		L"-Zi",L"-Qembed_debug", // デバック用の情報を埋め込む
		L"-Od", // 最適化を外しておく
		L"-Zpr", // メモリレイアウトは行優先
	};
	// 実際にShaderをコンパイルする
	Microsoft::WRL::ComPtr<IDxcResult> shaderResult = nullptr;
	hr = dxcCompiler->Compile(
		&shaderSourceBuffer, // 読み込んだファイル
		arguments, // コンパイルオプション
		_countof(arguments), // コンパイルオプションの数
		includeHandler, // includeが含まれた諸々
		IID_PPV_ARGS(&shaderResult) // コンパイル結果
	);
	// コンパイルエラーではなくdxcが起動できないなど致命的な状況
	assert(SUCCEEDED(hr));

	// 3.警告・エラーが出ていないか確認する
	// 警告・エラーが出てたらログを出して止める
	Microsoft::WRL::ComPtr<IDxcBlobUtf8> shaderError = nullptr;
	shaderResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&shaderError), nullptr);
	if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
		logger.Write(os, shaderError->GetStringPointer());
		// 警告・エラーダメ絶対
		assert(false);
	}

	//4.Compile結果を受け取って返す
	// コンパイル結果から実行用のバイナリ部分を取得
	Microsoft::WRL::ComPtr<IDxcBlob> shaderBlob = nullptr;
	hr = shaderResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&shaderBlob), nullptr);
	assert(SUCCEEDED(hr));
	// 成功したログを出す
	logger.Write(os, ConvertString(std::format(L"Compile Succeeded, path:{}, profile:{}\n", filePath, profile)));

	// ComPtrが自動でRelease()を呼ぶので、手動での解放は不要
	// shaderSource->Release();
	// shaderResult->Release();

	// 実行用のバイナリを返却
	return shaderBlob;
}

// ディスクリプタヒープの生成
Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible) {
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc{};
	descriptorHeapDesc.Type = heapType; // ディスクリプタヒープの種類。
	descriptorHeapDesc.NumDescriptors = numDescriptors; // ディスクリプタの数。スワップチェインの数と同じ。
	descriptorHeapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	HRESULT hr = device->CreateDescriptorHeap(
		&descriptorHeapDesc, // ディスクリプタヒープの設定
		IID_PPV_ARGS(&descriptorHeap)); // ディスクリプタヒープのポインタ
	// ディスクリプタヒープの生成がうまくいかなかったので起動できない
	assert(SUCCEEDED(hr));
	return descriptorHeap;
}

// DepthStencilTextureの生成
Microsoft::WRL::ComPtr<ID3D12Resource> CreateDepthStencilTextureResource(ID3D12Device* device, int32_t width, int32_t height) {
	// 1.生成するResourceの設定
	D3D12_RESOURCE_DESC resourceDesc{};
	resourceDesc.Width = width; // Textureの幅
	resourceDesc.Height = height; // Textureの高さ
	resourceDesc.MipLevels = 1; // MipMapの数
	resourceDesc.DepthOrArraySize = 1; // Textureの深さ
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // Textureのフォーマット
	resourceDesc.SampleDesc.Count = 1; // マルチサンプリングの数。1はマルチサンプリングなし
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // Textureの次元
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // DepthStencil用のフラグ

	// 2.利用するheapの設定
	D3D12_HEAP_PROPERTIES heapProperties{};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // ヒープの種類。DefaultHeapを使う

	// 3.深度値のクリア設定
	D3D12_CLEAR_VALUE depthClearValue{};
	depthClearValue.DepthStencil.Depth = 1.0f; // 深度値のクリア値
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // フォーマット

	// 4.Resourceの生成
	Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr; // Resourceのポインタ
	HRESULT hr = device->CreateCommittedResource(
		&heapProperties, // ヒープの設定
		D3D12_HEAP_FLAG_NONE, // ヒープのフラグ。特になし
		&resourceDesc, // Resourceの設定
		D3D12_RESOURCE_STATE_DEPTH_WRITE, // Resourceの初期状態。
		&depthClearValue, // 深度値のクリア設定
		IID_PPV_ARGS(&resource)); // Resourceのポインタ
	// Resourceの生成に失敗したので起動できない
	assert(SUCCEEDED(hr)); // 失敗したらassertで止める
	return resource; // Resourceのポインタを返す
}

// ディスクリプタヒープの先頭から指定したインデックスのCPUディスクリプタハンドルを取得する
D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index) {
	// ディスクリプタヒープの先頭を取得する
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	// ディスクリプタのサイズを取得する
	handleCPU.ptr += descriptorSize * index; // インデックス分だけずらす
	return handleCPU; // CPUディスクリプタハンドルを返す
}

// ディスクリプタヒープの先頭から指定したインデックスのGPUディスクリプタハンドルを取得する
D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, uint32_t descriptorSize, uint32_t index) {
	// ディスクリプタヒープの先頭を取得する
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	// ディスクリプタのサイズを取得する
	handleGPU.ptr += descriptorSize * index; // インデックス分だけずらす
	return handleGPU; // GPUディスクリプタハンドルを返す
}

// mtlファイルを読む関数
MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
	//1. 中で必要となる変数の宣言
	MaterialData materialData; // 構築するMaterialData
	std::string line; // ファイルから読んだ1行を格納するもの
	//2. ファイルを開く
	std::ifstream file(directoryPath + "/" + filename); // ファイルを開く
	assert(file.is_open()); // とりあえず開けなかったら止める
	//3. 実際にファイルを読み、MaterialDataを構築していく
	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;
		// identifierに応じた処理
		if (identifier == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;
			// 連結してファイルパスする
			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		}
	}
	//4. MaterialDataを返す
	return materialData;
}

//// Objファイルを読む関数
//ModelData LoadObjFile(const std::string& filename) {
//	//1. 中で必要となる変数の宣言
//	ModelData modelData; // 構築するModelData
//	std::vector<Vector4> positions; // 位置
//	std::vector<Vector3> normals; // 法線
//	std::vector<Vector2> texcoords; // テクスチャ座標
//	std::string line; // ファイルから読んだ1行を格納するもの
//	//2. ファイルを開く
//	std::filesystem::path fullPath(filename);
//	std::string directoryPath = fullPath.parent_path().string();
//	std::ifstream file(filename); // ファイルを開く
//	assert(file.is_open()); // とりあえず開けなかったら止める
//	//3. 実際にファイルを読み、ModelDataを構築していく
//	while (std::getline(file, line)) {
//		std::string identifier;
//		std::istringstream s(line);
//		s >> identifier; // 先頭の識別子を読む
//		// identifierに応じた処理
//		if (identifier == "v") {
//			Vector4 position{};
//			s >> position.x >> position.y >> position.z;
//			position.w = 1.0f;
//			positions.push_back(position);
//		} else if (identifier == "vt") {
//			Vector2 texcoord{};
//			s >> texcoord.x >> texcoord.y;
//			texcoords.push_back(texcoord);
//		} else if (identifier == "vn") {
//			Vector3 normal{};
//			s >> normal.x >> normal.y >> normal.z;
//			normals.push_back(normal);
//		} else if (identifier == "f") {
//			VertexData triangle[3];
//			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
//				std::string vertexDefinition;
//				s >> vertexDefinition;
//				std::istringstream v(vertexDefinition);
//
//				std::string indexStr;
//				uint32_t posIndex = 0, uvIndex = 0, normIndex = 0;
//
//				// v/t/n, v//n, v/t, v などに対応
//				if (std::getline(v, indexStr, '/')) {
//					posIndex = std::stoi(indexStr);
//				}
//
//				if (std::getline(v, indexStr, '/')) {
//					if (!indexStr.empty()) {
//						uvIndex = std::stoi(indexStr);
//					}
//				}
//
//				if (std::getline(v, indexStr, '/')) {
//					if (!indexStr.empty()) {
//						normIndex = std::stoi(indexStr);
//					}
//				}
//
//				// 実データ取得（存在するか確認）
//				Vector4 position = positions[posIndex - 1];
//				Vector2 texcoord = uvIndex > 0 ? texcoords[uvIndex - 1] : Vector2{ 0.0f, 0.0f };
//				Vector3 normal = normIndex > 0 ? normals[normIndex - 1] : Vector3{ 0.0f, 0.0f, 1.0f };
//
//				position.x *= -1.0f;
//				texcoord.y = 1.0f - texcoord.y;
//				normal.x *= -1.0f;
//
//				triangle[faceVertex] = { position, texcoord, normal };
//			}
//			// 反転して左手系 → 右手系
//			modelData.vertices.push_back(triangle[2]);
//			modelData.vertices.push_back(triangle[1]);
//			modelData.vertices.push_back(triangle[0]);
//		} else if (identifier == "mtllib") {
//			// materialTemplateLibraryファイルの名前を取得する
//			std::string materialFilename;
//			s >> materialFilename;
//			// 基本的にobjファイルと同一階層にmtlは存在させるので、ディレクトリ名とファイル名を渡す
//			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
//		}
//	}
//	//4. ModelDataを返す
//	return modelData;
//}

ModelData LoadObjFile(const std::string& filename) {
	Assimp::Importer importer;
	ModelData modelData;

	const aiScene* scene = importer.ReadFile(filename.c_str(), aiProcess_FlipWindingOrder | aiProcess_FlipUVs);
	assert(scene->HasMeshes());
	
	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
		aiMesh* mesh = scene->mMeshes[meshIndex];
		assert(mesh->HasNormals());
		assert(mesh->HasTextureCoords(0));

		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {
			aiFace& face = mesh->mFaces[faceIndex];
			assert(face.mNumIndices == 3);

			for (uint32_t element = 0; element < face.mNumIndices; ++element) {
				uint32_t vertexIndex = face.mIndices[element];
				aiVector3D& position = mesh->mVertices[vertexIndex];
				aiVector3D& normal = mesh->mNormals[vertexIndex];
				aiVector3D& texcoord = mesh->mTextureCoords[0][vertexIndex];
				VertexData vertex;
				vertex.position = { position.x, position.y, position.z, 1.0f };
				vertex.normal = { normal.x, normal.y, normal.z };
				vertex.texcoord = { texcoord.x, texcoord.y };

				vertex.position.x *= -1.0f;
				vertex.normal.x *= -1.0f;
				modelData.vertices.push_back(vertex);
			}
		}
	}

	for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex) {
		aiMaterial* material = scene->mMaterials[materialIndex];
		if (material->GetTextureCount(aiTextureType_DIFFUSE) != 0) {
			aiString textureFilePath;
			material->GetTexture(aiTextureType_DIFFUSE, 0, &textureFilePath);
			std::filesystem::path fullPath(filename);
			std::string directoryPath = fullPath.parent_path().string();
			modelData.material.textureFilePath = directoryPath + "/" + textureFilePath.C_Str();
		}
	}

	return modelData;
}
