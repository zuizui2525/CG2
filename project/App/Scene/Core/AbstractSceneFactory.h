#pragma once
#include <memory>
#include <string>
#include "App/Scene/Core/IScene.h"

/**
 * @brief シーン生成のための抽象工場（インターフェース）
 * * SceneManagerはこのクラスを介してシーンを生成します。
 * これにより、SceneManagerが具体的な「TitleScene」や「DebugScene」を知る必要がなくなります。
 */
class AbstractSceneFactory {
public:
    // 仮想デストラクタ：派生クラスの破棄を安全にするため
    virtual ~AbstractSceneFactory() = default;

    /**
     * @brief 指定された識別子（文字列）に対応するシーンを生成する
     * @param sceneName 生成したいシーンの名前
     * @return 生成されたシーンのユニークポインタ
     */
    virtual std::unique_ptr<IScene> CreateScene(const std::string& sceneName) = 0;
};
