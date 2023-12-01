#pragma once

#include <string>
#include <unordered_map>
#include "bvh.h"

namespace Tmpl8
{
    class BVHModel
    {
    public:
        BVHModel() {}
        BVHModel(const int idx, const std::string& modelPath, const mat4 transform, const mat4 scaleMat);
        void SetTransform(mat4 transform);
        void Intersect(Ray& ray);
        float3 GetNormal(const uint idx, const float2 barycentric) const;
        float2 GetUV(const uint idx, const float2 barycentric) const;
        Material* GetMaterial();
    public:
        int objIdx = -1;
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        Material material;
        mat4 T, invT;
        BVH bvh;
    };
}