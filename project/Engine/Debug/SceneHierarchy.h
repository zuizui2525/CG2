#pragma once
#include <vector>
#include <string>

class IGameObject;

/// <summary>
/// シーン内のすべてのデバッグ可能オブジェクトを走査・管理するシングルトンクラス
/// </summary>
class SceneHierarchy {
public:
    static SceneHierarchy* GetInstance();

    // 登録と名前の一意化
    std::string Register(IGameObject* object, const std::string& suggestedName);
    void Unregister(IGameObject* object);
    
    // SetName時のリネームと重複解決
    std::string Rename(IGameObject* object, const std::string& newName);

    // 登録オブジェクト一覧の取得
    const std::vector<IGameObject*>& GetObjects() const { return objects_; }

    // 現在エディタで選択されているオブジェクトの管理
    void SetSelected(IGameObject* obj) { selected_ = obj; }
    IGameObject* GetSelected() const { return selected_; }
    
    // 一括クリア
    void Clear();

private:
    SceneHierarchy() = default;
    ~SceneHierarchy() = default;

    // 名前の衝突を解決するヘルパー関数
    std::string ResolveUniqueName(IGameObject* self, const std::string& baseName);

    std::vector<IGameObject*> objects_;
    IGameObject* selected_ = nullptr;
};
