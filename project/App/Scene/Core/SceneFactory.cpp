#include "App/Scene/Core/SceneFactory.h"
#include "App/Scene/Title/TitleScene.h"
#include "App/Scene/Debug/DebugScene.h"

std::unique_ptr<IScene> SceneFactory::CreateScene(const std::string& sceneName) {
    std::unique_ptr<IScene> newScene = nullptr;

    if (sceneName == "Title") {
        newScene = std::make_unique<TitleScene>();
    } else if (sceneName == "Debug") {
        newScene = std::make_unique<DebugScene>();
    }

    return newScene;
}
