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
	public: 
        Model() {}
		Model(const std::string& path)
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
		}
        void Intersect(Ray& ray) const
        {
        }
        bool IsOccluded(const Ray& ray) const
        {
            return false;
        }
        float3 GetNormal(const float3 I) const
        {
            return float3(0);
        }
        float3 GetAlbedo(const float3 I) const
        {
            return float3(0);
        }
    public:
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
	};
}