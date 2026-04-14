#pragma once
#include <string>
#include <Windows.h>

/**
 * @brief string(UTF-8) -> wstring変換
 */
std::wstring ConvertString(const std::string& str);

/**
 * @brief wstring -> string(UTF-8)変換
 */
std::string ConvertString(const std::wstring& str);
