#pragma once

class ResourceLoader {
public:
    /// <summary>
    /// すべてのリソース（テクスチャ・モデルなど）のロードを一括で行う
    /// </summary>
    static void LoadAll();

private:
    /// <summary>
    /// 全てのテクスチャをロードする
    /// </summary>
    static void LoadTextures();

    /// <summary>
    /// 全てのモデルをロードする
    /// </summary>
    static void LoadModels();
};
