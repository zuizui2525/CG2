#pragma once
#include "Struct.h"

// 球同士の当たり判定
bool IsCollision(const Sphere& sphere1, const Sphere& sphere2);
// 球と面の当たり判定
bool IsCollision(const Sphere& sphere, const Plane& plane);
// 線と平面の当たり判定
bool IsCollision(const Segment& segment, const Plane& plane);
// 線と三角形の当たり判定
bool IsCollision(const Segment& segment, const Triangle& triangle);
// AABB同士の当たり判定
bool IsCollision(const AABB& a, const AABB& b);
// AABBと点の当たり判定
bool IsCollision(const AABB& aabb, const Vector3& position);
// AABBと球の当たり判定
bool IsCollision(const AABB& aabb, const Sphere& sphere);
// AABBと線の当たり判定
bool IsCollision(const AABB& aabb, const Segment& segment);
// OBBと球の当たり判定
bool IsCollision(const OBB& obb, const Sphere& sphere);
// OBBと線の当たり判定
bool IsCollision(const OBB& obb, const Segment& segment);
