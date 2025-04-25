#pragma once
#include "Struct.h"
// 行列の転置(matrix4x4)
Matrix4x4 Transpose(const Matrix4x4& m);
// 逆行列(Matrix4x4)
Matrix4x4 Inverse(const Matrix4x4& m);
// 単位行列(Matrix4x4)
Matrix4x4 MakeIdentity4x4();

// 平行移動行列(Matrix4x4)
Matrix4x4 MakeTranslateMatrix(const Vector3& translate);
// 拡大縮小行列(Matrix4x4)
Matrix4x4 MakeScaleMatrix(const Vector3& scale);
// 回転行列(Matrix4x4)
// X軸回転行列
Matrix4x4 MakeRotateXMatrix(float radian);
// Y軸回転行列
Matrix4x4  MakeRotateYMatrix(float radian);
// Z軸回転行列
Matrix4x4  MakeRotateZMatrix(float radian);
// アフィン変換行列(Matrix4x4)
Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, Vector3& translate);

// 投視投影行列
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip);
// 正射影行列
Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);
// ビューポート変換行列
Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth);

// クロス積
Vector3 Cross(const Vector3& v1, const Vector3& v2);
// Matrix4x4の加算
Matrix4x4 Add(const Matrix4x4& m1, const Matrix4x4& m2);
// Matrix4x4の減算
Matrix4x4 Subtract(const Matrix4x4& m1, const Matrix4x4& m2);
// Matrix4x4の乗算
Matrix4x4 Multiply(const Matrix4x4& a, const Matrix4x4& b);
// 座標変換(Matrix4x4からVector3へ)
Vector3 TransformMatrix4x4ToVector3(const Matrix4x4& matrix, const Vector3& vector);
