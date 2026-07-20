#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <shellapi.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <format>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")

namespace fs = std::filesystem;

namespace {
    constexpr int kPort = 8080;
    std::string s_reportPath = "";

    // 最も新しいレポートフォルダを自動で探す
    std::string FindLatestReport() {
        std::string target = "out/performance_reports";
        if (!fs::exists(target)) {
            return "";
        }

        fs::path latestPath;
        std::time_t latestTime = 0;

        for (const auto& entry : fs::directory_iterator(target)) {
            if (entry.is_directory()) {
                auto writeTime = fs::last_write_time(entry);
                // ファイル更新時間をstd::time_tに変換（簡易的）
                auto sct = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    writeTime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
                std::time_t t = std::chrono::system_clock::to_time_t(sct);

                if (t > latestTime) {
                    latestTime = t;
                    latestPath = entry.path();
                }
            }
        }
        return latestPath.string();
    }

    std::string GetMimeType(const std::string& extension) {
        if (extension == ".html") return "text/html; charset=utf-8";
        if (extension == ".css") return "text/css; charset=utf-8";
        if (extension == ".js") return "application/javascript; charset=utf-8";
        if (extension == ".json") return "application/json; charset=utf-8";
        if (extension == ".bmp") return "image/bmp";
        if (extension == ".png") return "image/png";
        if (extension == ".mp4") return "video/mp4";
        if (extension == ".md") return "text/markdown; charset=utf-8";
        return "application/octet-stream";
    }

    // クライアントにファイルを送る関数
    void SendFile(SOCKET clientSocket, const std::string& filePath, const std::string& mimeType) {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            std::string response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
            send(clientSocket, response.c_str(), (int)response.size(), 0);
            return;
        }

        // ファイルサイズ取得
        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        // レスポンスヘッダー送信
        std::string header = std::format(
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: {}\r\n"
            "Content-Length: {}\r\n"
            "Access-Control-Allow-Origin: *\r\n"
            "Connection: close\r\n\r\n",
            mimeType, fileSize
        );
        send(clientSocket, header.c_str(), (int)header.size(), 0);

        // ファイルデータ送信 (1024バイトずつバッファリング)
        std::vector<char> buffer(4096);
        while (file.read(buffer.data(), buffer.size()) || file.gcount() > 0) {
            send(clientSocket, buffer.data(), (int)file.gcount(), 0);
        }
    }
}

int main(int argc, char* argv[]) {
    std::cout << "=========================================================" << std::endl;
    std::cout << "   ZuizuiEngine Performance Report Viewer Server Started " << std::endl;
    std::cout << "=========================================================" << std::endl;

    // 1. レポートフォルダの決定
    if (argc > 1) {
        s_reportPath = argv[1];
    } else {
        s_reportPath = FindLatestReport();
    }

    if (s_reportPath.empty() || !fs::exists(s_reportPath)) {
        std::cerr << "[エラー] レポートフォルダが見つかりません。" << std::endl;
        std::cerr << "         ゲーム側でパフォーマンスバグを発生させるか、" << std::endl;
        std::cerr << "         フォルダパスを引数に指定して起動してください。" << std::endl;
        std::cout << "\n終了するには Enter を押してください...";
        std::cin.get();
        return 1;
    }

    std::cout << "[INFO] 対象データ: " << s_reportPath << std::endl;

    // 2. WinSockの初期化
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "[エラー] Winsockの初期化に失敗しました。" << std::endl;
        return 1;
    }

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "[エラー] ソケットの作成に失敗しました。" << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(kPort);

    // ポートのバインド
    if (bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "[エラー] ポート " << kPort << " のバインドに失敗しました。" << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // リッスン開始
    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "[エラー] ソケットの監視開始に失敗しました。" << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // 3. ブラウザ自動起動
    std::string url = std::format("http://localhost:{}/", kPort);
    std::cout << "[INFO] ブラウザを起動しています: " << url << std::endl;
    ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);

    std::cout << "\n>> サーバー動作中... 終了するにはブラウザの [CLOSE] を押すか、" << std::endl;
    std::cout << ">> このコンソールで [Ctrl + C] を押してください。" << std::endl;

    // 4. HTTPリクエストループ
    bool running = true;
    while (running) {
        SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            break;
        }

        // リクエストヘッダーの読み取り (簡易)
        std::vector<char> reqBuffer(2048, 0);
        int bytesReceived = recv(clientSocket, reqBuffer.data(), (int)reqBuffer.size() - 1, 0);
        if (bytesReceived > 0) {
            std::string request(reqBuffer.data());
            std::stringstream ss(request);
            std::string method, rawPath;
            ss >> method >> rawPath;

            if (method == "GET") {
                // パスの正規化
                if (rawPath == "/") {
                    rawPath = "/index.html";
                }

                // 終了API
                if (rawPath == "/exit") {
                    std::cout << "[INFO] ブラウザから終了リクエストを受信しました。シャットダウンします。" << std::endl;
                    std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
                    send(clientSocket, response.c_str(), (int)response.size(), 0);
                    closesocket(clientSocket);
                    running = false;
                    break;
                }

                std::string targetFilePath;
                std::string ext = fs::path(rawPath).extension().string();

                // アプリケーションのアセットか、ダンプデータかでパスを分岐
                if (rawPath == "/index.html" || rawPath == "/style.css" || rawPath == "/app.js") {
                    // ツール内アセット（Tools/PerformanceViewer 内）
                    targetFilePath = "project/Tools/PerformanceViewer" + rawPath;
                } else if (rawPath.find("/data/") == 0) {
                    // ダンプデータ（/data/system_log.json ➔ out/performance_reports/report_.../system_log.json）
                    std::string subPath = rawPath.substr(5); // "/data/" の後ろ
                    targetFilePath = s_reportPath + "/" + subPath;
                }

                if (!targetFilePath.empty() && fs::exists(targetFilePath)) {
                    SendFile(clientSocket, targetFilePath, GetMimeType(ext));
                } else {
                    std::string response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
                    send(clientSocket, response.c_str(), (int)response.size(), 0);
                }
            }
        }
        closesocket(clientSocket);
    }

    // 5. シャットダウン
    closesocket(listenSocket);
    WSACleanup();
    std::cout << "[INFO] サーバーを終了しました。" << std::endl;
    return 0;
}
