#include "App/Scene/Game/Stage/StageEditor.h"
#include "Engine/Debug/SceneHierarchy.h"
#include "Engine/Debug/GameViewWindow.h"
#include <imgui.h>
#include <fstream>
#include <sstream>
#include <direct.h> // CreateDirectoryA用

void StageEditor::Initialize(std::vector<std::unique_ptr<Enemy>>* enemies) {
    enemies_ = enemies;
    selectedEnemyIndex_ = -1;
    showStageEditor_ = true;
}

void StageEditor::Update() {
    if (!enemies_) return;

    // ギズモでオブジェクトが選択された場合の双方向同期
    auto selectedObj = SceneHierarchy::GetInstance()->GetSelected();
    if (selectedObj) {
        bool found = false;
        for (int i = 0; i < (int)enemies_->size(); ++i) {
            if ((*enemies_)[i]->GetCube() == selectedObj) {
                selectedEnemyIndex_ = i;
                found = true;
                break;
            }
        }
        if (!found) {
            // 敵以外のオブジェクトが選択された場合はリストの選択を外す
            selectedEnemyIndex_ = -1;
        }
    } else {
        selectedEnemyIndex_ = -1;
    }
}

void StageEditor::ImGuiControl() {
#ifdef _USEIMGUI
    if (!showStageEditor_ || !enemies_) return;

    ImGui::Begin("Stage Editor");
    
    // 敵の追加
    if (ImGui::Button("Add Enemy")) {
        auto enemy = std::make_unique<Enemy>();
        enemy->Initialize();
        enemy->SetPosition({ 0.0f, 1.0f, 0.0f });
        enemy->SetSpawnPoint(true);
        enemy->SetSize({ 1.0f, 0.1f, 3.0f }); // size.xに出現数1を設定
        enemies_->push_back(std::move(enemy));
        selectedEnemyIndex_ = (int)enemies_->size() - 1;
        
        // ギズモのターゲットに設定
        SceneHierarchy::GetInstance()->SetSelected((*enemies_).back()->GetCube());
    }

    ImGui::Separator();
    ImGui::Text("Enemies List:");
    
    // 敵のリスト表示
    for (int i = 0; i < (int)enemies_->size(); ++i) {
        std::string label = "Enemy " + std::to_string(i);
        bool isSelected = (selectedEnemyIndex_ == i);
        if (ImGui::Selectable(label.c_str(), isSelected)) {
            selectedEnemyIndex_ = i;
            SceneHierarchy::GetInstance()->SetSelected((*enemies_)[i]->GetCube());
        }
    }

    // 選択中の敵の編集
    if (selectedEnemyIndex_ >= 0 && selectedEnemyIndex_ < (int)enemies_->size()) {
        ImGui::Separator();
        ImGui::Text("Selected Spawn Point:");
        auto& enemy = (*enemies_)[selectedEnemyIndex_];
        Vector3 pos = enemy->GetPosition();
        Vector3 size = enemy->GetSize();
        
        if (ImGui::DragFloat3("Position", &pos.x, 0.1f)) {
            enemy->SetPosition(pos);
        }
        
        int spawnCount = static_cast<int>(size.x);
        if (ImGui::SliderInt("Spawn Count", &spawnCount, 1, 5)) {
            size.x = static_cast<float>(spawnCount);
            enemy->SetSize(size);
        }
        
        if (ImGui::Button("Delete Point")) {
            if (SceneHierarchy::GetInstance()->GetSelected() == enemy->GetCube()) {
                SceneHierarchy::GetInstance()->SetSelected(nullptr);
            }
            enemies_->erase(enemies_->begin() + selectedEnemyIndex_);
            selectedEnemyIndex_ = -1;
        }
    }

    // セーブ・ロード
    ImGui::Separator();
    if (ImGui::Button("Save Stage")) {
        SaveStage(kStageFilePath);
    }
    ImGui::SameLine();
    if (ImGui::Button("Load Stage")) {
        LoadStage(kStageFilePath);
    }
    ImGui::End();
#endif
}

void StageEditor::SaveStage(const std::string& filepath) {
    if (!enemies_) return;

    // ディレクトリ作成
    _mkdir("resources");
    _mkdir("resources/stages");

    std::ofstream ofs(filepath);
    if (!ofs.is_open()) return;

    ofs << "[\n";
    for (size_t i = 0; i < enemies_->size(); ++i) {
        auto& enemy = (*enemies_)[i];
        Vector3 pos = enemy->GetPosition();
        Vector3 size = enemy->GetSize();
        ofs << "  {\"pos\": [" << pos.x << ", " << pos.y << ", " << pos.z << "], "
            << "\"size\": [" << size.x << ", " << size.y << ", " << size.z << "]}";
        if (i + 1 < enemies_->size()) {
            ofs << ",";
        }
        ofs << "\n";
    }
    ofs << "]\n";
}

void StageEditor::LoadStage(const std::string& filepath) {
    if (!enemies_) return;

    std::ifstream ifs(filepath);
    if (!ifs.is_open()) return;

    enemies_->clear();
    selectedEnemyIndex_ = -1;
    SceneHierarchy::GetInstance()->SetSelected(nullptr);

    std::string line;
    while (std::getline(ifs, line)) {
        size_t posIdx = line.find("\"pos\": [");
        if (posIdx == std::string::npos) continue;

        size_t posEnd = line.find("]", posIdx);
        if (posEnd == std::string::npos) continue;

        std::string posStr = line.substr(posIdx + 8, posEnd - (posIdx + 8));
        float px = 0.0f, py = 0.0f, pz = 0.0f;
        if (sscanf_s(posStr.c_str(), "%f, %f, %f", &px, &py, &pz) != 3) continue;

        size_t sizeIdx = line.find("\"size\": [");
        float sx = 1.0f, sy = 1.0f, sz = 1.0f;
        if (sizeIdx != std::string::npos) {
            size_t sizeEnd = line.find("]", sizeIdx);
            if (sizeEnd != std::string::npos) {
                std::string sizeStr = line.substr(sizeIdx + 9, sizeEnd - (sizeIdx + 9));
                sscanf_s(sizeStr.c_str(), "%f, %f, %f", &sx, &sy, &sz);
            }
        }

        auto enemy = std::make_unique<Enemy>();
        enemy->Initialize();
        enemy->SetPosition({ px, py, pz });
        enemy->SetSpawnPoint(true);
        enemy->SetSize({ sx, sy, sz });
        enemies_->push_back(std::move(enemy));
    }
}
