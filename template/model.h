#pragma once

#include <string>
#include <unordered_map>

// take the most famous vulkan learning tutorial I leanred as the reference

struct Tri
{
    Tri() {};
    Tri(float3 v0, float3 v1, float3 v2, float3 n0, float3 n1, float3 n2, float3 c, int idx)
        : vertex0(v0), vertex1(v1), vertex2(v2), normal0(n0), normal1(n1), normal2(n2), centroid(c), objIdx(idx)
    {};
    float3 vertex0, vertex1, vertex2;
    float3 normal0, normal1, normal2;
    float3 centroid;
    int objIdx;
};

struct Vertex
{
    float3 position{};
    float3 normal{};
    float2 uv{};

    bool operator==(const Vertex& other) const
    {
        return position == other.position && normal == other.normal && uv == other.uv;
    }
};

namespace std
{
    template <class T>
    inline void hash_combine(std::size_t& s, const T& v)
    {
        std::hash<T> h;
        s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
    }

    template <>
    struct hash<float2>
    {
        size_t operator()(float2 const& v) const
        {
            std::size_t res = 0;
            hash_combine(res, v.x);
            hash_combine(res, v.y);
            return res;
        }
    };

    template <>
    struct hash<float3>
    {
        size_t operator()(float3 const& v) const
        {
            std::size_t res = 0;
            hash_combine(res, v.x);
            hash_combine(res, v.y);
            hash_combine(res, v.z);
            return res;
        }
    };

    template <>
    struct hash<Vertex>
    {
        size_t operator()(Vertex const& vertex) const
        {
            return ((hash<float3>()(vertex.position) ^
                (hash<float3>()(vertex.normal) << 1)) >>
                1) ^
                (hash<float2>()(vertex.uv) << 1);
        }
    };
}
namespace Tmpl8
{
	class Model
	{
    private:
        void IntersectTri(Ray& ray, const Vertex& vertex0, const Vertex& vertex1, const Vertex& vertex2, const int& idx) const;
        bool IsOccludedTri(const Ray& ray, const Vertex& vertex0, const Vertex& vertex1, const Vertex& vertex2) const;
	public: 
        Model() {}
        Model(const int idx, const std::string& path, mat4 transform);
        void Intersect(Ray& ray) const;
        bool IsOccluded(const Ray& ray) const;
        // https://www.khronos.org/opengl/wiki/Calculating_a_Surface_Normal
        float3 GetNormal(const int triIdx) const;
        Material* GetMaterial();
        void AppendTriangles(std::vector<Tri>& triangles);
    public:
        int objIdx = -1;
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        Material material;
        mat4 T, invT;
	};
}