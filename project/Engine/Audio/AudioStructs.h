#pragma once
#include <Windows.h>
#include <xaudio2.h>
#include <cstdint>
#include <vector>

// 音声再生用データ
struct SoundData {
    std::vector<BYTE> audioData;                //!< 音声データ本体
    WAVEFORMATEX* wfex = nullptr;               //!< フォーマット情報
    IXAudio2SourceVoice* sourceVoice = nullptr; //!< ソースボイス
};
