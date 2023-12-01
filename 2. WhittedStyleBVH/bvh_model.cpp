#include "precomp.h"
#include "bvh_model.h"

BVHModel::BVHModel(const int idx, const std::string& modelPath, const mat4 transform, const mat4 scaleMat)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, modelPath.c_str()))
    {
        throw std::runtime_error(warn + err);
    }

    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    for (const auto& shape : shapes)
    {
        for (const auto& index : shape.mesh.indices)
        {
            Vertex vertex{};

            if (index.vertex_index >= 0)
            {
                vertex.position = {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2] };
            }

            if (index.normal_index >= 0)
            {
                vertex.normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2] };
            }

            if (index.texcoord_index >= 0)
            {
                vertex.uv = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    attrib.texcoords[2 * index.texcoord_index + 1] };
            }

            if (uniqueVertices.count(vertex) == 0)
            {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }
            indices.push_back(uniqueVertices[vertex]);
        }
    }

    T = transform;
    invT = transform.FastInvertedTransformNoScale();

    objIdx = idx;

    std::vector<Tri> triangles;
    for (int i = 0; i < indices.size(); i += 3)
    {
        Tri tri(
            TransformPosition(vertices[indices[i]].position, scaleMat),
            TransformPosition(vertices[indices[i + 1]].position, scaleMat),
            TransformPosition(vertices[indices[i + 2]].position, scaleMat),
            vertices[indices[i]].normal,
            vertices[indices[i + 1]].normal,
            vertices[indices[i + 2]].normal,
            vertices[indices[i]].uv,
            vertices[indices[i + 1]].uv,
            vertices[indices[i + 2]].uv,
            float3(0), objIdx);
        tri.centroid = (tri.vertex0 + tri.vertex1 + tri.vertex2) * 0.3333f;
        triangles.push_back(tri);
    }
    bvh.SetTriangles(triangles);

    bvh.Build();
}

void BVHModel::Intersect(Ray& ray)
{
    Ray tRay = Ray(ray);
    tRay.O = TransformPosition_SSE(ray.O4, invT);
    tRay.D = TransformVector_SSE(ray.D4, invT);
    tRay.rD = float3(1 / ray.D.x, 1 / ray.D.y, 1 / ray.D.z);

    bvh.Intersect(tRay);

    tRay.O = ray.O;
    tRay.D = ray.D;
    tRay.rD = ray.rD;
    ray = tRay;
}

float3 BVHModel::GetNormal(const uint idx, const float2 barycentric) const
{
    float3 N = bvh.GetNormal(idx, barycentric);
    return normalize(TransformVector(N, invT));
}

float2 BVHModel::GetUV(const uint idx, const float2 barycentric) const
{
    return bvh.GetUV(idx, barycentric);
}

Material* BVHModel::GetMaterial()
{
    return &material;
}

void BVHModel::SetTransform(mat4 transform)
{
    T = transform;
    invT = transform.FastInvertedTransformNoScale();
    // update bvh bound
    // calculate world-space bounds using the new matrix
    float3 bmin = bvh.bvhNodes[0].aabbMin, bmax = bvh.bvhNodes[0].aabbMax;
    bvh.bounds = aabb();
    for (int i = 0; i < 8; i++)
        bvh.bounds.Grow(TransformPosition(float3(i & 1 ? bmax.x : bmin.x,
            i & 2 ? bmax.y : bmin.y, i & 4 ? bmax.z : bmin.z), transform));
}