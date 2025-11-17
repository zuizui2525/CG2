#include "Audio.h"
#include "Function/Function.h"
#include <iostream>

Audio::Audio() {}
Audio::~Audio() { Finalize(); }

void Audio::Initialize() {
    // COM と Media Foundation の初期化
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    MFStartup(MF_VERSION);

    // XAudio2の初期化
    HRESULT hr = XAudio2Create(&xAudio2_, 0, XAUDIO2_DEFAULT_PROCESSOR);
    assert(SUCCEEDED(hr));

    hr = xAudio2_->CreateMasteringVoice(&masterVoice_);
    assert(SUCCEEDED(hr));
}

void Audio::Finalize() {
    if (masterVoice_) {
        masterVoice_->DestroyVoice();
        masterVoice_ = nullptr;
    }
    xAudio2_.Reset();

    MFShutdown();
    CoUninitialize();
}

SoundData Audio::LoadSound(const std::string& filePath) {
    // UTF-8 → UTF-16変換
    std::wstring wFilePath = ConvertString(filePath);

    SoundData soundData{};
    Microsoft::WRL::ComPtr<IMFSourceReader> reader;
    HRESULT hr = MFCreateSourceReaderFromURL(wFilePath.c_str(), nullptr, &reader);
    assert(SUCCEEDED(hr));

    // PCM形式に変換して読み込む
    Microsoft::WRL::ComPtr<IMFMediaType> audioType;
    MFCreateMediaType(&audioType);
    audioType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
    audioType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
    reader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, audioType.Get());

    // 出力フォーマット情報を取得
    Microsoft::WRL::ComPtr<IMFMediaType> outputType;
    reader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_AUDIO_STREAM, &outputType);

    UINT32 blockAlign = 0;
    UINT32 bitsPerSample = 0;
    UINT32 samplesPerSec = 0;
    UINT32 channels = 0;
    outputType->GetUINT32(MF_MT_AUDIO_BLOCK_ALIGNMENT, &blockAlign);
    outputType->GetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, &bitsPerSample);
    outputType->GetUINT32(MF_MT_AUDIO_SAMPLES_PER_SECOND, &samplesPerSec);
    outputType->GetUINT32(MF_MT_AUDIO_NUM_CHANNELS, &channels);

    // WAVEFORMATEX構築
    WAVEFORMATEX* wfex = new WAVEFORMATEX();
    wfex->wFormatTag = WAVE_FORMAT_PCM;
    wfex->nChannels = static_cast<WORD>(channels);
    wfex->nSamplesPerSec = samplesPerSec;
    wfex->wBitsPerSample = static_cast<WORD>(bitsPerSample);
    wfex->nBlockAlign = static_cast<WORD>(blockAlign);
    wfex->nAvgBytesPerSec = wfex->nSamplesPerSec * wfex->nBlockAlign;
    wfex->cbSize = 0;

    soundData.wfex = wfex;

    // 音声データを読み取り
    Microsoft::WRL::ComPtr<IMFSample> sample;
    Microsoft::WRL::ComPtr<IMFMediaBuffer> buffer;
    DWORD flags = 0;
    while (SUCCEEDED(reader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr, &flags, nullptr, &sample)) && sample) {
        if (flags & MF_SOURCE_READERF_ENDOFSTREAM) break;
        sample->ConvertToContiguousBuffer(&buffer);

        BYTE* audioPtr = nullptr;
        DWORD audioSize = 0;
        buffer->Lock(&audioPtr, nullptr, &audioSize);

        size_t start = soundData.audioData.size();
        soundData.audioData.resize(start + audioSize);
        memcpy(&soundData.audioData[start], audioPtr, audioSize);

        buffer->Unlock();
        buffer.Reset();
        sample.Reset();
    }

    return soundData;
}

void Audio::PlaySound(SoundData& soundData, bool loop) {
    if (!xAudio2_ || !soundData.wfex) return;

    HRESULT hr = xAudio2_->CreateSourceVoice(&soundData.sourceVoice, soundData.wfex);
    assert(SUCCEEDED(hr));

    XAUDIO2_BUFFER buffer{};
    buffer.AudioBytes = static_cast<UINT32>(soundData.audioData.size());
    buffer.pAudioData = soundData.audioData.data();
    buffer.Flags = XAUDIO2_END_OF_STREAM;
    buffer.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;

    hr = soundData.sourceVoice->SubmitSourceBuffer(&buffer);
    assert(SUCCEEDED(hr));
    soundData.sourceVoice->Start(0);
}

void Audio::StopSound(SoundData& soundData) {
    if (soundData.sourceVoice) {
        soundData.sourceVoice->Stop();
        soundData.sourceVoice->FlushSourceBuffers();
    }
}

void Audio::Unload(SoundData& soundData) {
    if (soundData.sourceVoice) {
        soundData.sourceVoice->DestroyVoice();
        soundData.sourceVoice = nullptr;
    }
    delete soundData.wfex;
    soundData.wfex = nullptr;
    soundData.audioData.clear();
}
