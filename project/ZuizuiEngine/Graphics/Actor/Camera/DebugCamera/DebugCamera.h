#pragma once
#include "Function.h"
#include "Input.h"

class DebugCamera {
private:
	Vector3 rotation_ = {};
	Vector3 translation_ = { 0.0f, 0.0f, -10.0f };
	Matrix4x4 viewMatrix_;
	Matrix4x4 projectionMatrix_;

	float moveSpeed_ = 0.1f; // 移動速度
	// 回転速度係数（感度）
	const float rotateSpeed = 0.001f;
	// カメラの前方ベクトルと右方向ベクトルを保持
	Vector3 forwardVector_ = { 0.0f, 0.0f, 1.0f }; // 初期の前方
	Vector3 rightVector_ = { 1.0f, 0.0f, 0.0f };   // 初期の右方向
	Vector3 upVector_ = { 0.0f, 1.0f, 0.0f };      // 初期の上方向（Y軸固定）
	HWND hwnd_ = nullptr; // 追加：ウィンドウハンドル
	bool isCursorHidden_ = false;
	void ResetPosition();
public:
	bool skipNextMouseUpdate_ = false; // 初回回転スキップ用
	void Initialize();
	void Update(Input* input);
	const Matrix4x4& GetViewMatrix() { return viewMatrix_; }
	const Matrix4x4& GetProjectionMatrix() { return projectionMatrix_; }
	// ウィンドウハンドル設定
	void HideCursor();
	void ShowCursorBack();
};
