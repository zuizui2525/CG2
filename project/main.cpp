#include "App.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    // リークチェック
    D3DResourceLeakChecker leakCheck;
    SetUnhandledExceptionFilter(ExportDump);

    // App生成
    std::unique_ptr<App> app = std::make_unique<App>();

    // 初期化
    app->Initialize();

    // メインループ: 終わるまでRunを繰り返す
    while (!app->IsEnd()) {
        app->Run();
    }

    // 終了
    app->Finalize();
    return 0;
}
