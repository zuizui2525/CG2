#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include "Struct.h"

class MapChipField {
public:
    // マップチップの種類
    enum class MapChipType {
        kBlank,     // 空白
        kBlock,     // ブロック
        kDeadBlock, // 針・落下など
    };

    struct MapChipData {
        std::vector<std::vector<MapChipType>> data;
    };

    struct IndexSet {
        uint32_t xIndex;
        uint32_t yIndex;
    };

    struct Rect {
        float left;
        float right;
        float bottom;
        float top;
    };

public:
    // 定数取得
    static float GetBlockWidth() { return kBlockWidth_; }
    static float GetBlockHeight() { return kBlockHeight_; }
    uint32_t GetNumBlockVertical() { return kNumBlockVertical_; }
    uint32_t GetNumBlockHorizontal() { return kNumBlockHorizontal_; }

    // 主要機能
    void ResetMapChipData();
    void LoadMapChipCsv(const std::string& filePath);

    // インデックス・座標変換
    MapChipType GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex);
    Vector3 GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex);
    IndexSet GetMapChipIndexSetByPosition(const Vector3& position);
    Rect GetRectByIndex(uint32_t xIndex, uint32_t yIndex);

private:
    // 1ブロックのサイズ
    static inline const float kBlockWidth_ = 0.2f;
    static inline const float kBlockHeight_ = 0.2f;
    // ブロックの最大個数（CSVの想定サイズ）
    static inline const uint32_t kNumBlockVertical_ = 20;
    static inline const uint32_t kNumBlockHorizontal_ = 100;

    MapChipData mapChipData_;
};
