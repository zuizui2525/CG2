#pragma once
#include "AbstractSceneFactory.h"

/**
 * @brief 具体的なシーン生成工場
 */
class SceneFactory : public AbstractSceneFactory {
public:
    // 抽象クラスの関数をオーバーライド
    std::unique_ptr<IScene> CreateScene(const std::string& sceneName) override;
};
