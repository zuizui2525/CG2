#include "App/Scene/Game/Route.h"
#include "Engine/Input/Input.h"
#include "Engine/Graphics/Objects/Camera/Manager/CameraManager.h"
#include "Engine/Graphics/Objects/Camera/Base/BaseCamera.h"
#include "Engine/Math/Matrix/Matrix.h"
#include "Engine/Base/WindowApp/WindowApp.h"
#include "Engine/Debug/GameViewWindow.h"
#include <cmath>
#include <algorithm>

Route::Route() {
    startSphere_ = std::make_unique<SphereObject>();
    goalSphere_ = std::make_unique<SphereObject>();
}

void Route::Initialize(Input* input, CameraManager* cameraMgr) {
    input_ = input;
    cameraMgr_ = cameraMgr;

    // スタート地点とゴール地点の視覚用球体オブジェクトの初期化
    startSphere_->Initialize();
    startSphere_->SetScale({ kAreaRadius * 2.0f, kAreaRadius * 2.0f, kAreaRadius * 2.0f });
    startSphere_->SetColor({ 1.0f, 1.0f, 0.0f, 0.5f }); // 黄色（半透明）

    goalSphere_->Initialize();
    goalSphere_->SetScale({ kAreaRadius * 2.0f, kAreaRadius * 2.0f, kAreaRadius * 2.0f });
    goalSphere_->SetColor({ 0.0f, 0.5f, 1.0f, 0.5f }); // 青色（半透明）

    Reset();
}

void Route::Reset() {
    ClearForNewArea();
    SetupArea(0);
}

void Route::Update(BaseCamera* activeCamera) {
    // マウスドラッグによるルートの記録
    if (input_->MousePress(0)) { // 左クリック
        Vector2 mousePos = GameViewWindow::GetMousePosition();
        Vector2 viewSize = GameViewWindow::GetGameViewSize();

        // 画面全体の30%の左側領域のみ操作を受け付ける
        static const float kLeftAreaRate = 0.3f;
        if (mousePos.x <= viewSize.x * kLeftAreaRate) {
            Vector3 rayStart, rayDir;
            activeCamera->CreateRay(mousePos, viewSize.x * kLeftAreaRate, viewSize.y, rayStart, rayDir);

            if (std::abs(rayDir.y) > 0.0001f) {
                float t = (kPlaneIntersectY - rayStart.y) / rayDir.y;
                if (t >= 0.0f) {
                    Vector3 intersectPos = Math::Add(rayStart, Math::Multiply(t, rayDir));

                    // マップ範囲内に入っているかチェック
                    if (std::abs(intersectPos.x) <= kMapBoundaryX && intersectPos.z >= currentAreaStartZ_ && intersectPos.z <= currentAreaGoalZ_) {
                        if (rawPoints_.empty()) {
                            // 最初の一点はスタートエリア付近のみ許可
                            float toStartX = intersectPos.x - 0.0f;
                            float toStartZ = intersectPos.z - currentAreaStartZ_;
                            float distToStartSq = toStartX * toStartX + toStartZ * toStartZ;

                            if (distToStartSq <= kAreaRadius * kAreaRadius) {
                                ClearForNewArea();
                                isDrawing_ = true;
                                rawPoints_.push_back(intersectPos);
                            }
                        } else {
                            if (!hasReachedGoal_) {
                                Vector3 diff = Math::Subtract(intersectPos, rawPoints_.back());
                                float dist = Math::Length(diff);
                                if (dist >= kMinPointDistance) {
                                    // ゴールエリアに到達したかチェック
                                    float toGoalX = intersectPos.x - 0.0f;
                                    float toGoalZ = intersectPos.z - currentAreaGoalZ_;
                                    float distToGoalSq = toGoalX * toGoalX + toGoalZ * toGoalZ;

                                    rawPoints_.push_back(intersectPos);

                                    auto line = std::make_unique<LineObject>();
                                    line->Initialize(0);
                                    line->SetStartPoint(rawPoints_[rawPoints_.size() - 2]);
                                    line->SetEndPoint(rawPoints_.back());
                                    line->SetThickness(kLineThickness);
                                    line->SetColor(kLineColor);
                                    lineObjects_.push_back(std::move(line));

                                    // ゴールエリアに入ったら終了
                                    if (distToGoalSq <= kAreaRadius * kAreaRadius) {
                                        hasReachedGoal_ = true;
                                        isDrawing_ = false;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        isDrawing_ = false;
    }
}

void Route::Update2D(const Vector3& intersectPos) {
    if (input_->MousePress(0)) {
        if (std::abs(intersectPos.x) <= kMapBoundaryX && intersectPos.z >= currentAreaStartZ_ && intersectPos.z <= currentAreaGoalZ_) {
            // ドラッグ開始時にスタート位置の近くをクリックした場合は線をクリアして書き直す
            if (input_->MouseTrigger(0)) {
                float toStartX = intersectPos.x - 0.0f;
                float toStartZ = intersectPos.z - currentAreaStartZ_;
                float distToStartSq = toStartX * toStartX + toStartZ * toStartZ;
                if (distToStartSq <= kAreaRadius * kAreaRadius) {
                    ClearForNewArea();
                }
            }

            if (rawPoints_.empty()) {
                ClearForNewArea();
                isDrawing_ = true;
                
                // 1点目はスタート地点固定
                Vector3 startPos = { 0.0f, 0.0f, currentAreaStartZ_ };
                rawPoints_.push_back(startPos);
                
                // 2点目として現在のドラッグ位置を追加
                rawPoints_.push_back(intersectPos);

                auto line = std::make_unique<LineObject>();
                line->Initialize(0);
                line->SetStartPoint(startPos);
                line->SetEndPoint(intersectPos);
                line->SetThickness(kLineThickness);
                line->SetColor(kLineColor);
                lineObjects_.push_back(std::move(line));
            } else {
                if (!hasReachedGoal_) {
                    Vector3 diff = Math::Subtract(intersectPos, rawPoints_.back());
                    float dist = Math::Length(diff);
                    if (dist >= kMinPointDistance) {
                        float toGoalX = intersectPos.x - 0.0f;
                        float toGoalZ = intersectPos.z - currentAreaGoalZ_;
                        float distToGoalSq = toGoalX * toGoalX + toGoalZ * toGoalZ;

                        rawPoints_.push_back(intersectPos);

                        auto line = std::make_unique<LineObject>();
                        line->Initialize(0);
                        line->SetStartPoint(rawPoints_[rawPoints_.size() - 2]);
                        line->SetEndPoint(rawPoints_.back());
                        line->SetThickness(kLineThickness);
                        line->SetColor(kLineColor);
                        lineObjects_.push_back(std::move(line));

                        if (distToGoalSq <= kAreaRadius * kAreaRadius) {
                            hasReachedGoal_ = true;
                            isDrawing_ = false;
                        }
                    }
                }
            }
        }
    } else {
        isDrawing_ = false;
    }

    for (auto& line : lineObjects_) {
        line->Update();
    }
    for (auto& line : editorGizmoLines_) {
        line->Update();
    }
    startSphere_->Update();
    goalSphere_->Update();
}

void Route::UpdateSpheres() {
    startSphere_->Update();
    goalSphere_->Update();
}

void Route::Draw() {
    for (auto& line : editorGizmoLines_) {
        line->Draw();
    }
    for (const auto& line : lineObjects_) {
        line->Draw();
    }
}

void Route::DrawSpheres() {
    startSphere_->Draw();
    goalSphere_->Draw();
}

void Route::FinalizeRoute() {
    const int kPathDivision = 40; // 視覚的なカクつきをなくすため分割数を40に増加

    // rawPoints_のコピーを作成し、移動平均フィルタを複数回（5回）適用して急激なジグザグ・手ブレ角を丸める
    std::vector<Vector3> smoothedPoints = rawPoints_;
    if (smoothedPoints.size() >= 3) {
        for (int iter = 0; iter < 5; ++iter) {
            std::vector<Vector3> temp = smoothedPoints;
            for (size_t i = 1; i < smoothedPoints.size() - 1; ++i) {
                temp[i].x = (smoothedPoints[i - 1].x + smoothedPoints[i].x + smoothedPoints[i + 1].x) / 3.0f;
                temp[i].y = (smoothedPoints[i - 1].y + smoothedPoints[i].y + smoothedPoints[i + 1].y) / 3.0f;
                temp[i].z = (smoothedPoints[i - 1].z + smoothedPoints[i].z + smoothedPoints[i + 1].z) / 3.0f;
            }
            smoothedPoints = temp;
        }
    }

    pathPoints_ = Math::GenerateCatmullRomPath(smoothedPoints, kPathDivision);

    BuildEqualSpacingTable();
}

void Route::BuildEqualSpacingTable() {
    accumDistances_.clear();
    accumDistances_.push_back(0.0f);
    float accum = 0.0f;
    for (size_t i = 1; i < pathPoints_.size(); ++i) {
        float dist = Math::Length(Math::Subtract(pathPoints_[i], pathPoints_[i - 1]));
        accum += dist;
        accumDistances_.push_back(accum);
    }
    totalDistance_ = accum;
}

Vector3 Route::GetPositionAtDistance(float distance) const {
    if (pathPoints_.empty()) return { 0.0f, 0.0f, 0.0f };
    if (distance <= 0.0f) return pathPoints_.front();
    if (distance >= totalDistance_) return pathPoints_.back();

    size_t idx = 0;
    for (size_t i = 0; i < accumDistances_.size() - 1; ++i) {
        if (accumDistances_[i] <= distance && distance < accumDistances_[i + 1]) {
            idx = i;
            break;
        }
    }

    float tLocal = 0.0f;
    float distDiff = accumDistances_[idx + 1] - accumDistances_[idx];
    if (distDiff > 0.0001f) {
        tLocal = (distance - accumDistances_[idx]) / distDiff;
    }

    return Math::Add(pathPoints_[idx], Math::Multiply(tLocal, Math::Subtract(pathPoints_[idx + 1], pathPoints_[idx])));
}

Vector3 Route::GetTangentAtDistance(float distance) const {
    if (pathPoints_.size() < 2) return { 0.0f, 0.0f, 1.0f };
    
    float targetDist = std::clamp(distance, 0.0f, totalDistance_);
    size_t idx = 0;
    for (size_t i = 0; i < accumDistances_.size() - 1; ++i) {
        if (accumDistances_[i] <= targetDist && targetDist < accumDistances_[i + 1]) {
            idx = i;
            break;
        }
    }
    if (idx >= pathPoints_.size() - 1) idx = pathPoints_.size() - 2;

    return Math::Normalize(Math::Subtract(pathPoints_[idx + 1], pathPoints_[idx]));
}

Vector3 Route::GetRotationAtDistance(float distance) const {
    Vector3 tangent = GetTangentAtDistance(distance);
    float yaw = std::atan2(tangent.x, tangent.z);
    float pitch = -std::atan2(tangent.y, std::sqrt(tangent.x * tangent.x + tangent.z * tangent.z));
    return { pitch, yaw, 0.0f };
}

void Route::AddGizmoRect(const Vector3& center, float width, float depth, const Vector4& color) {
    float hx = width * kHalf;
    float hz = depth * kHalf;

    Vector3 corners[4] = {
        { center.x - hx, 0.01f, center.z - hz },
        { center.x + hx, 0.01f, center.z - hz },
        { center.x + hx, 0.01f, center.z + hz },
        { center.x - hx, 0.01f, center.z + hz }
    };

    for (int i = 0; i < 4; ++i) {
        auto line = std::make_unique<LineObject>();
        line->Initialize(0);
        line->SetStartPoint(corners[i]);
        line->SetEndPoint(corners[(i + 1) % 4]);
        line->SetThickness(kGizmoThickness);
        line->SetColor(color);
        editorGizmoLines_.push_back(std::move(line));
    }
}

void Route::AddGizmoCircle(const Vector3& center, float radius, const Vector4& color) {
    std::vector<Vector3> points;
    points.reserve(kCircleDivision);
    for (int i = 0; i < kCircleDivision; ++i) {
        float theta = (2.0f * kPi * i) / kCircleDivision;
        float x = center.x + radius * std::cos(theta);
        float z = center.z + radius * std::sin(theta);
        points.push_back({ x, 0.01f, z });
    }

    for (int i = 0; i < kCircleDivision; ++i) {
        auto line = std::make_unique<LineObject>();
        line->Initialize(0);
        line->SetStartPoint(points[i]);
        line->SetEndPoint(points[(i + 1) % kCircleDivision]);
        line->SetThickness(kGizmoThickness);
        line->SetColor(color);
        editorGizmoLines_.push_back(std::move(line));
    }
}

void Route::SetupArea(int areaIndex) {
    currentAreaIndex_ = areaIndex;
    
    // エリアごとの定数
    static const float kAreaLength = 120.0f;
    currentAreaStartZ_ = kStartAreaZ + static_cast<float>(areaIndex) * kAreaLength;
    currentAreaGoalZ_ = currentAreaStartZ_ + kAreaLength;

    // スタート地点とゴール地点の球体座標更新
    startSphere_->SetPosition({ 0.0f, 1.0f, currentAreaStartZ_ });
    goalSphere_->SetPosition({ 0.0f, 1.0f, currentAreaGoalZ_ });

    SetupAreaGizmos();
}

void Route::SetupAreaGizmos() {
    editorGizmoLines_.clear();

    // エリア境界枠（白）: Xはマップ左右外枠、Zは現在のエリア範囲
    float centerZ = (currentAreaStartZ_ + currentAreaGoalZ_) * kHalf;
    float lengthZ = currentAreaGoalZ_ - currentAreaStartZ_;
    AddGizmoRect({ 0.0f, 0.0f, centerZ }, kMapBoundaryX * 2.0f, lengthZ, { 1.0f, 1.0f, 1.0f, 1.0f });

    // スタート枠 (黄・円形)
    AddGizmoCircle({ 0.0f, 0.0f, currentAreaStartZ_ }, kAreaRadius, { 1.0f, 1.0f, 0.0f, 1.0f });
    // ゴール枠 (青・円形)
    AddGizmoCircle({ 0.0f, 0.0f, currentAreaGoalZ_ }, kAreaRadius, { 0.0f, 0.5f, 1.0f, 1.0f });

    // ボス出現ライン (エリア3のZ=360fに配置)
    static const float kBossSpawnLineZ = 180.0f;
    if (currentAreaIndex_ == 3) {
        // 赤い太めの横線を引く
        auto line = std::make_unique<LineObject>();
        line->Initialize(0);
        line->SetStartPoint({ -kMapBoundaryX, 0.01f, kBossSpawnLineZ });
        line->SetEndPoint({ kMapBoundaryX, 0.01f, kBossSpawnLineZ });
        static const float kBossLineThickness = 0.5f;
        line->SetThickness(kBossLineThickness);
        line->SetColor({ 1.0f, 0.0f, 0.0f, 1.0f }); // 赤色
        editorGizmoLines_.push_back(std::move(line));
    }
}

void Route::ClearForNewArea() {
    rawPoints_.clear();
    pathPoints_.clear();
    accumDistances_.clear();
    lineObjects_.clear();
    totalDistance_ = 0.0f;
    isDrawing_ = false;
    hasReachedGoal_ = false;
}

void Route::UpdateLines() {
    for (auto& line : lineObjects_) {
        line->Update();
    }
    for (auto& line : editorGizmoLines_) {
        line->Update();
    }
}

void Route::SyncFrom(const Route* other) {
    currentAreaIndex_ = other->currentAreaIndex_;
    currentAreaStartZ_ = other->currentAreaStartZ_;
    currentAreaGoalZ_ = other->currentAreaGoalZ_;
    isDrawing_ = other->isDrawing_;
    hasReachedGoal_ = other->hasReachedGoal_;
    rawPoints_ = other->rawPoints_;
    pathPoints_ = other->pathPoints_;
    accumDistances_ = other->accumDistances_;
    totalDistance_ = other->totalDistance_;

    // LineObject 群 (手書きルート線) の同期
    if (lineObjects_.size() != other->lineObjects_.size()) {
        lineObjects_.clear();
        for (size_t i = 0; i < other->lineObjects_.size(); ++i) {
            auto line = std::make_unique<LineObject>();
            line->Initialize();
            line->SetColor(kLineColor);
            line->SetThickness(kLineThickness);
            lineObjects_.push_back(std::move(line));
        }
    }
    // 座標の同期
    for (size_t i = 0; i < lineObjects_.size(); ++i) {
        lineObjects_[i]->SetStartPoint(other->lineObjects_[i]->GetStartPoint());
        lineObjects_[i]->SetEndPoint(other->lineObjects_[i]->GetEndPoint());
    }

    // 球体の同期
    startSphere_->SetPosition(other->startSphere_->GetPosition());
    goalSphere_->SetPosition(other->goalSphere_->GetPosition());
}
