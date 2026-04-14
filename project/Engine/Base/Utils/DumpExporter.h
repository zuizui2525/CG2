#pragma once
#include <Windows.h>
#include <dbghelp.h>

/**
 * @brief 例外発生時にミニダンプを出力する関数
 * @param exception 例外情報
 * @return 実行結果
 */
LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception);
