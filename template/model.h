#pragma once

#include <string>
#include <unordered_map>

// take the most famous vulkan learning tutorial I leanred as the reference

struct Vertex
{
    float3 pos;
    float3 color;
    float2 texCoord;

    bool operator==(const Vertex& other) const
    {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
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
            return ((hash<float3>()(vertex.pos) ^
                (hash<float3>()(vertex.color) << 1)) >>
                1) ^
                (hash<float2>()(vertex.texCoord) << 1);
        }
    };
}
namespace Tmpl8
{
	class Model
	{
    private:
        void IntersectTri(Ray& ray, const Vertex& vertex0, const Vertex& vertex1, const Vertex& vertex2, const int& idx) const
        {
            const float3 v0 = TransformPosition(vertex0.pos, M);
            const float3 v1 = TransformPosition(vertex1.pos, M);
            const float3 v2 = TransformPosition(vertex2.pos, M);
            const float3 edge1 = v1 - v0;
            const float3 edge2 = v2 - v0;
            const float3 h = cross(ray.D, edge2);
            const float a = dot(edge1, h);
            if (a > -0.0001f && a < 0.0001f) return; // ray parallel to triangle
            const float f = 1 / a;
            const float3 s = ray.O - v0;
            const float u = f * dot(s, h);
            if (u < 0 || u > 1) return;
            const float3 q = cross(s, edge1);
            const float v = f * dot(ray.D, q);
            if (v < 0 || u + v > 1) return;
            const float t = f * dot(edge2, q);
            if (t > 0.0001f)
            {
                if (t < ray.t) ray.t = min(ray.t, t), ray.objIdx = idx;
            }
        }
	public: 
        Model() {}
		Model(const std::string& path, mat4 transform = mat4::Identity())
		{
            tinyobj::attrib_t attrib;
            std::vector<tinyobj::shape_t> shapes;
            std::vector<tinyobj::material_t> materials;
            std::string warn, err;

            if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str()))
            {
                throw std::runtime_error(warn + err);
            }

            std::unordered_map<Vertex, uint32_t> uniqueVertices{};

            for (const auto& shape : shapes)
            {
                for (const auto& index : shape.mesh.indices)
                {
                    Vertex vertex{};

                    vertex.pos = {
                        attrib.vertices[3 * index.vertex_index + 0],
                        attrib.vertices[3 * index.vertex_index + 1],
                        attrib.vertices[3 * index.vertex_index + 2] };

                    vertex.texCoord = {
                        attrib.texcoords[2 * index.texcoord_index + 0],
                        attrib.texcoords[2 * index.texcoord_index + 1] };

                    vertex.color = { 1.0f, 1.0f, 1.0f };

                    if (uniqueVertices.count(vertex) == 0)
                    {
                        uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                        vertices.push_back(vertex);
                    }
                    indices.push_back(uniqueVertices[vertex]);
                }
            }

            M = transform;
		}
        void Intersect(Ray& ray) const
        {
            for (int i = 0; i < indices.size(); i+=3)
            {
                IntersectTri(
                    ray, 
                    vertices[indices[i]], vertices[indices[i+1]], vertices[indices[i+2]], i);
            }
        }
        bool IsOccluded(const Ray& ray) const
        {
            return false;
        }
        float3 GetNormal(const float3 I) const
        {
            return float3(0);
        }
        Material GetMaterial() const
        {
            return material;
        }
    public:
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        Material material;
        mat4 M;
	};
}