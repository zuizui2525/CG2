#include "Engine/Debug/IGameObject.h"
#include "Engine/Debug/SceneHierarchy.h"

IGameObject::~IGameObject() {
    // 破棄時に自動的にヒエラルキーから登録解除する
    SceneHierarchy::GetInstance()->Unregister(this);
}

void IGameObject::InitializeGameObject(const std::string& defaultName) {
    // ヒエラルキーへ登録し、被らない名前を自動決定して保持
    name_ = SceneHierarchy::GetInstance()->Register(this, defaultName);
}

void IGameObject::SetName(const std::string& name) {
    // 名前が変更された際も重複をチェックして更新
    name_ = SceneHierarchy::GetInstance()->Rename(this, name);
}
