#pragma once
#include <xaudio2.h>
#include <wrl.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <string>
#include <vector>
#include <cassert>

struct SoundData {
    std::vector<BYTE> audioData;     //!< 音声データ本体
    WAVEFORMATEX* wfex = nullptr;    //!< フォーマット情報
    IXAudio2SourceVoice* sourceVoice = nullptr; //!< ソースボイス
};

class Audio {
public:
    Audio();
    ~Audio();

    // 初期化と終了処理
    void Initialize();
    void Finalize();

    // 音声読み込み (.wav / .mp3対応)
    SoundData LoadSound(const std::string& filePath);

    // 再生・停止・解放
    void PlaySound(SoundData& soundData, bool loop = false);
    void StopSound(SoundData& soundData);
    void Unload(SoundData& soundData);

private:
    Microsoft::WRL::ComPtr<IXAudio2> xAudio2_;
    IXAudio2MasteringVoice* masterVoice_ = nullptr;
};
