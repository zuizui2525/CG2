#pragma once
#include <vector>
#include <memory>
#include <string>
#include "App/Scene/Game/Enemy/Enemy.h"

/**
 * @brief ステージ上の敵の配置、ImGuiエディタUI、セーブ・ロードを管理するエディタクラス
 */
class StageEditor {
public:
    StageEditor() = default;
    ~StageEditor() = default;

    void Initialize(std::vector<std::unique_ptr<Enemy>>* enemies);
    void Update();
    void ImGuiControl();

    // ステージ保存・読み込み
    void SaveStage(const std::string& filepath);
    void LoadStage(const std::string& filepath);

    // ゲッター/セッター
    bool IsShowEditor() const { return showStageEditor_; }
    void SetShowEditor(bool show) { showStageEditor_ = show; }
    bool* GetShowEditorPtr() { return &showStageEditor_; }

private:
    std::vector<std::unique_ptr<Enemy>>* enemies_ = nullptr; // 敵オブジェクトリストのポインタ
    int selectedEnemyIndex_ = -1;                          // 選択中の敵のインデックス
    bool showStageEditor_ = true;                          // ステージエディタウィンドウの表示フラグ

    static inline const std::string kStageFilePath = "resources/stages/stage1.json"; // ステージ保存先パス
    static inline const std::string kTextureKey = "white"; // 敵テクスチャ
};
