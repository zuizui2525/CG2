#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <format>
#include <Windows.h>

class Log {
public:
    Log() = default;
    ~Log() = default;

    // 出力関数（ファイル＋デバッグ出力）
    static void Write(const std::string& message);
    static void Write(const std::wstring& message);

    // ostream（例：std::cout）にも出力
    static void Write(std::ostream& os, const std::string& message);
    static void Write(std::ostream& os, const std::wstring& message);

private:
    static void Initialize();

    static std::ofstream logStream_;   // ログファイル出力用
    static std::string logFileName_;   // ログファイル名
    static bool isInitialized_;         // 初期化フラグ
    static std::chrono::steady_clock::time_point startTime_; // 起動時の基準時間
};
