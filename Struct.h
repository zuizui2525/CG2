#pragma once

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