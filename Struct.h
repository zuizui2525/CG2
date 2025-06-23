#pragma once

// Vector2
struct Vector2 {
	float x;
	float y;
};

// Vector3
struct Vector3 {
	float x;
	float y;
	float z;
};

//Vector4
struct Vector4 {
	float x;
	float y;
	float z;
	float w;
};

//Matrix4x4
struct Matrix4x4 {
	float m[4][4];
};

// Transform
struct Transform {
	Vector3 scale;
	Vector3 rotate;
	Vector3 translate;
};

struct VertexData {
	Vector4 position;
	Vector2 texcoord;
};

struct Sphere {
	Vector3 center; //!< 中心点
	float radius; //!< 半径
};