#include "Engine/Base/Utils/DumpExporter.h"
#include <strsafe.h>
#include <objbase.h>

#pragma comment(lib, "Dbghelp.lib")

// ダンプファイルの生成
LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception) {
	// COMの初期化
	CoInitializeEx(0, COINIT_MULTITHREADED);
	// 時刻を取得して、時刻を名前に入れたファイルを作成。Dumpsディレクトリ以下に出力
	SYSTEMTIME time;
	GetLocalTime(&time);
	wchar_t filePath[MAX_PATH] = { 0 };
	CreateDirectory(L"./Dumps", nullptr);
	StringCchPrintfW(filePath, MAX_PATH, L"./Dumps/%04d-%02d%02d-%02d%02d.dmp", time.wYear, time.wYear, time.wDay, time.wHour, time.wMinute);
	HANDLE dumpFileHandle = CreateFile(
		filePath, // ファイル名
		GENERIC_READ | GENERIC_WRITE, // 読み書き
		FILE_SHARE_WRITE | FILE_SHARE_READ, // 書き込みと読み込みの共有
		0, // セキュリティ属性
		CREATE_ALWAYS, // 常に新規作成
		0, // 属性
		0); // テンプレートファイル
	// processId(このexeのId)とクラッシュ(例外)の発生したthreadIdを取得
	DWORD processId = GetCurrentProcessId();
	DWORD threadId = GetCurrentThreadId();
	// 設定情報を入力
	MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{ 0 };
	minidumpInformation.ThreadId = threadId; // スレッドID
	minidumpInformation.ExceptionPointers = exception; // 例外情報
	minidumpInformation.ClientPointers = TRUE; // クライアントポインタを使う
	// Dumpを出力。MiniDumpNormalは最小限の情報を出力するフラグ
	MiniDumpWriteDump(
		GetCurrentProcess(), // プロセスハンドル
		processId, // プロセスID
		dumpFileHandle, // ダンプファイルハンドル
		MiniDumpNormal, // ダンプの種類
		&minidumpInformation, // 例外情報
		nullptr, // スレッド情報
		nullptr); // ユーザーデータ
	// 他に関連図けられているSEH例外ハンドラがあれば実行。通常はプロセスを終了する
	return EXCEPTION_EXECUTE_HANDLER;
}
