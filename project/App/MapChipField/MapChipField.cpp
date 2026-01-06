#include "MapChipField.h"
#include <map>
#include <fstream>
#include <sstream>
#include <cassert>

namespace {
    // CSV内の文字と列挙型の紐付けテーブル
    std::map<std::string, MapChipField::MapChipType> mapChipTable = {
        {"0", MapChipField::MapChipType::kBlank},
        {"1", MapChipField::MapChipType::kBlock},
        {"2", MapChipField::MapChipType::kDeadBlock},
    };
}

void MapChipField::ResetMapChipData() {
    mapChipData_.data.clear();
    mapChipData_.data.resize(kNumBlockVertical_);
    for (auto& line : mapChipData_.data) {
        line.resize(kNumBlockHorizontal_, MapChipType::kBlank);
    }
}

void MapChipField::LoadMapChipCsv(const std::string& filePath) {
    ResetMapChipData();

    std::ifstream file(filePath);
    assert(file.is_open() && "Failed to open MapChip CSV file.");

    std::string line;
    for (uint32_t i = 0; i < kNumBlockVertical_ && std::getline(file, line); ++i) {
        std::istringstream line_stream(line);
        std::string word;

        for (uint32_t j = 0; j < kNumBlockHorizontal_ && std::getline(line_stream, word, ','); ++j) {
            if (mapChipTable.contains(word)) {
                mapChipData_.data[i][j] = mapChipTable[word];
            }
        }
    }
    file.close();
}

MapChipField::MapChipType MapChipField::GetMapChipTypeByIndex(uint32_t xIndex, uint32_t yIndex) {
    if (xIndex >= kNumBlockHorizontal_ || yIndex >= kNumBlockVertical_) {
        return MapChipType::kBlank;
    }
    return mapChipData_.data[yIndex][xIndex];
}

Vector3 MapChipField::GetMapChipPositionByIndex(uint32_t xIndex, uint32_t yIndex) {
    // Y軸は上が正、インデックスは下が正のため反転計算を行う
    return Vector3(
        kBlockWidth_ * (float)xIndex,
        kBlockHeight_ * (float)(kNumBlockVertical_ - 1 - yIndex),
        0.0f
    );
}

MapChipField::IndexSet MapChipField::GetMapChipIndexSetByPosition(const Vector3& position) {
    IndexSet indexSet = {};
    indexSet.xIndex = static_cast<uint32_t>((position.x + kBlockWidth_ / 2.0f) / kBlockWidth_);

    float yFromBottom = position.y + kBlockHeight_ / 2.0f;
    uint32_t yIndexFromBottom = static_cast<uint32_t>(yFromBottom / kBlockHeight_);
    indexSet.yIndex = kNumBlockVertical_ - 1 - yIndexFromBottom;

    return indexSet;
}

MapChipField::Rect MapChipField::GetRectByIndex(uint32_t xIndex, uint32_t yIndex) {
    Vector3 center = GetMapChipPositionByIndex(xIndex, yIndex);
    return Rect{
        center.x - kBlockWidth_ / 2.0f,
        center.x + kBlockWidth_ / 2.0f,
        center.y - kBlockHeight_ / 2.0f,
        center.y + kBlockHeight_ / 2.0f
    };
}
