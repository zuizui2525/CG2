#pragma once
#include "MathStructs.h"

struct Segment {
    Vector3 origin;     //!< 始点
    Vector3 diff;       //!< 終点への差分ベクトル
    unsigned int color; //!< 色
};

struct Triangle {
    Vector3 vertex[3];  //!< 頂点
    unsigned int color; //!< 色
};

struct Plane {
    Vector3 normal;     //!< 法線
    float distance;     //!< 距離
    unsigned int color; //!< 色
};

struct Sphere {
    Vector3 center;     //!< 中心点
    float radius;       //!< 半径
    unsigned int color; //!< 色
};

struct AABB {
    Vector3 min;        //!< 最小点
    Vector3 max;        //!< 最大点
    unsigned int color; //!< 色
};

struct OBB {
    Vector3 center;          //!< 中心点
    Vector3 orientations[3]; //!< 座標軸「。正規化・直交必須
    Vector3 size;            //!< 座標軸方向の長さの半分。中心から面までの距離
    unsigned int color;      //!< 色
};
