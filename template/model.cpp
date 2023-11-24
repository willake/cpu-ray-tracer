#include "precomp.h"

Model::Model(const int idx, const std::string& path, mat4 transform = mat4::Identity())
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

    objIdx = idx;
}

void Model::IntersectTri(Ray& ray, const Vertex& vertex0, const Vertex& vertex1, const Vertex& vertex2, const int& idx) const
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
        if (t < ray.t) ray.t = min(ray.t, t), ray.objIdx = objIdx, ray.triIdx = idx;
    }
}

bool Model::IsOccludedTri(const Ray& ray, const Vertex& vertex0, const Vertex& vertex1, const Vertex& vertex2) const
{
    const float3 v0 = TransformPosition(vertex0.pos, M);
    const float3 v1 = TransformPosition(vertex1.pos, M);
    const float3 v2 = TransformPosition(vertex2.pos, M);
    const float3 edge1 = v1 - v0;
    const float3 edge2 = v2 - v0;
    const float3 h = cross(ray.D, edge2);
    const float a = dot(edge1, h);
    if (a > -0.0001f && a < 0.0001f) return false; // ray parallel to triangle
    const float f = 1 / a;
    const float3 s = ray.O - v0;
    const float u = f * dot(s, h);
    if (u < 0 || u > 1) return false;
    const float3 q = cross(s, edge1);
    const float v = f * dot(ray.D, q);
    if (v < 0 || u + v > 1) return false;
    const float t = f * dot(edge2, q);
    if (t > 0.0001f)
    {
        return true;
    }
    return false;
}

void Model::Intersect(Ray& ray) const
{
    for (int i = 0; i < indices.size(); i += 3)
    {
        IntersectTri(
            ray,
            vertices[indices[i]], vertices[indices[i + 1]], vertices[indices[i + 2]], i);
    }
}

bool Model::IsOccluded(const Ray& ray) const
{
    for (int i = 0; i < indices.size(); i += 3)
    {
        if (IsOccludedTri(ray, vertices[indices[i]], vertices[indices[i + 1]], vertices[indices[i + 2]]))
        {
            return true;
        }
    }
    return false;
}

float3 Model::GetNormal(const int triIdx) const
{
    const float3 v0 = TransformPosition(vertices[indices[triIdx]].pos, M);
    const float3 v1 = TransformPosition(vertices[indices[triIdx + 1]].pos, M);
    const float3 v2 = TransformPosition(vertices[indices[triIdx + 2]].pos, M);
    const float3 u = v1 - v0;
    const float3 v = v2 - v0;
    return float3(
        u.y * v.z - u.z * v.y,
        u.z * v.x - u.x * v.z,
        u.x * v.y - u.y * v.x
    );
}

Material* Model::GetMaterial()
{
    return &material;
}

void Model::AppendTriangles(std::vector<Tri>& triangles)
{
    for (int i = 0; i < indices.size(); i += 3)
    {
        Tri tri(
            TransformPosition(vertices[indices[i]].pos, M),
            TransformPosition(vertices[indices[i + 1]].pos, M),
            TransformPosition(vertices[indices[i + 2]].pos, M),
            float3(0), objIdx);
        tri.centroid = (tri.vertex0 + tri.vertex1 + tri.vertex2) * 0.3333f;
        triangles.push_back(tri);
    }
}