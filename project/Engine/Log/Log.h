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
    // コンストラクタでログファイル生成
    Log();

    // デストラクタでファイルクローズ
    ~Log();

    // 出力関数（ファイル＋デバッグ出力）
    void Write(const std::string& message);

    // ostream（例：std::cout）にも出力
    void Write(std::ostream& os, const std::string& message);

    std::ofstream& GetLogStream() { return logStream_; };
private:
    std::ofstream logStream_;   // ログファイル出力用
    std::string logFileName_;   // ログファイル名
};
