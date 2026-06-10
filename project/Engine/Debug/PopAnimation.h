#ifdef _USEIMGUI
#pragma once
#include <cmath>
#include <algorithm>
#include "externals/imgui/imgui.h"

struct PopAnimation {
    enum class Type { None, Play, Pause };
    Type type = Type::None;
    float timer = 0.0f;
    float duration = 0.5f;

    void Trigger(Type t) {
        type = t;
        timer = duration;
    }

    void Update(float deltaTime) {
        if (timer > 0.0f) {
            timer -= deltaTime;
            if (timer <= 0.0f) {
                type = Type::None;
            }
        }
    }

    void Draw(ImDrawList* drawList, const ImVec2& center) const {
        if (type == Type::None || timer <= 0.0f) return;

        // 進捗率 (0.0f: 開始 -> 1.0f: 終了)
        float t = (duration - timer) / duration;
        t = std::clamp(t, 0.0f, 1.0f);

        // Cubic Ease Out イージング (徐々に減速して広がる)
        float invT = 1.0f - t;
        float t_eased = 1.0f - (invT * invT * invT);

        // スケールとアルファの算出
        constexpr float kStartRadius = 25.0f;
        constexpr float kEndRadius = 55.0f;
        float currentRadius = kStartRadius + t_eased * (kEndRadius - kStartRadius);

        // アルファフェードアウト (後半にかけてスッと消えるように2乗をかける)
        float alpha = invT * invT;

        // 各要素の不透明度 (YouTube風)
        const ImU32 kCircleColor = IM_COL32(0, 0, 0, static_cast<int>(alpha * 110.0f));
        const ImU32 kMarkColor = IM_COL32(255, 255, 255, static_cast<int>(alpha * 200.0f));

        // 1. 背景の円を描画
        constexpr int32_t kCircleSegments = 36;
        drawList->AddCircleFilled(center, currentRadius, kCircleColor, kCircleSegments);

        // 基準サイズ
        constexpr float kBaseSize = 15.0f;
        float scale = 1.0f + t_eased * 0.8f; // 1.0倍から1.8倍に拡大

        if (type == Type::Play) {
            // 2. 三角形 (▶) の描画
            float h = kBaseSize * scale;
            float H = h * 0.866f;
            
            // センタリングの補正値
            float xOff = -1.5f * scale;
            float yOff = -1.0f * scale;

            ImVec2 p1(center.x - H * 0.333f + xOff, center.y - h * 0.5f + yOff);
            ImVec2 p2(center.x - H * 0.333f + xOff, center.y + h * 0.5f + yOff);
            ImVec2 p3(center.x + H * 0.667f + xOff, center.y + yOff);

            drawList->AddTriangleFilled(p1, p2, p3, kMarkColor);
        } else if (type == Type::Pause) {
            // 3. 2本縦棒 (||) の描画
            float h = kBaseSize * scale;
            constexpr float kBaseBarWidth = 4.0f;
            constexpr float kBaseBarGap = 6.0f;
            float w = kBaseBarWidth * scale;
            float gap = kBaseBarGap * scale;

            // 左棒
            ImVec2 lMin(center.x - gap * 0.5f - w, center.y - h * 0.5f);
            ImVec2 lMax(center.x - gap * 0.5f, center.y + h * 0.5f);
            drawList->AddRectFilled(lMin, lMax, kMarkColor);

            // 右棒
            ImVec2 rMin(center.x + gap * 0.5f, center.y - h * 0.5f);
            ImVec2 rMax(center.x + gap * 0.5f + w, center.y + h * 0.5f);
            drawList->AddRectFilled(rMin, rMax, kMarkColor);
        }
    }
};
#endif
