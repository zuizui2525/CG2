#pragma once
#include "Engine/Graphics/RenderStructs.h"
#include <string>

/// @brief モデルの読み込みを担当するクラス
class ModelLoader {
public:
	/// @brief マテリアルテンプレートファイル (.mtl) を読み込む
	/// @param directoryPath ファイルが存在するディレクトリ
	/// @param filename ファイル名
	/// @return 読み込んだマテリアルデータ
	static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);

	/// @brief モデルファイル (.obj 等) を読み込む
	/// @param filename ファイル名
	/// @return 読み込んだモデルデータ
	static ModelData LoadObjFile(const std::string& filename);
};
