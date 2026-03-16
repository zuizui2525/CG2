#include "SceneFactory.h"
#include "TitleScene.h"
#include "DebugScene.h"

std::unique_ptr<IScene> SceneFactory::CreateScene(const std::string& sceneName) {
    std::unique_ptr<IScene> newScene = nullptr;

    if (sceneName == "Title") {
        newScene = std::make_unique<TitleScene>();
    } else if (sceneName == "Debug") {
        newScene = std::make_unique<DebugScene>();
    }

    return newScene;
}
