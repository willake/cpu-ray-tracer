#include "precomp.h"
#include "kdtree.h"

KDTree::KDTree(const int idx, const std::string& modelPath, const mat4 transform, const mat4 scaleMat)
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

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

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

    objIdx = idx;

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

    Build();
    SetTransform(transform);
}

void KDTree::Build()
{
    triangleBounds.resize(triangles.size());
    triangleIndices.resize(triangles.size());
    // populate triangle index array
    for (int i = 0; i < triangles.size(); i++)
    {
        // setup indices
        triangleIndices[i] = i;
    }
    UpdateBounds();
    // assign all triangles to root node
    rootNode = new KDTreeNode();
    rootNode->aabbMin = localBounds.bmin3;
    rootNode->aabbMax = localBounds.bmax3;
    rootNode->triIndices = triangleIndices;
    // subdivide recursively
    Subdivide(rootNode, 0);
}

void KDTree::UpdateBounds()
{
    aabb b;
    for (uint i = 0; i < triangleIndices.size(); i++)
    {
        uint triIdx = triangleIndices[i];
        Tri& tri = triangles[triIdx];
        aabb triBounds;
        triBounds.Grow(tri.vertex0);
        triBounds.Grow(tri.vertex1);
        triBounds.Grow(tri.vertex2);
        b.Grow(triBounds);
        triangleBounds[i] = triBounds;
    }
    localBounds = b;
}

void KDTree::Subdivide(KDTreeNode* node, int depth)
{
    printf("Node tris: %d Depth: %d \n", node->triIndices.size(), depth);
    // terminate recursion
    if (depth >= m_maxBuildDepth) return;
    uint triCount = node->triIndices.size();
    if (triCount <= 2) return;

    // split plane axis and position
    float3 extent = node->aabbMax - node->aabbMin;
    int axis = 0;
    if (extent.y > extent.x) axis = 1;
    if (extent.z > extent[axis]) axis = 2;
    float distance = extent[axis] * 0.5f;
    float splitPos = node->aabbMin[axis] + distance;

    std::vector<uint> leftTriIdxs;
    std::vector<uint> rightTriIdxs;

    for (int i = 0; i < triCount; i++)
    {
        uint idx = node->triIndices[i];
        if (triangleBounds[idx].bmin[axis] < splitPos)
        {
            leftTriIdxs.push_back(idx);
        }
        if (triangleBounds[idx].bmax[axis] > splitPos - 0.001)
        {
            rightTriIdxs.push_back(idx);
        }
    }

    if (leftTriIdxs.size() == triCount || rightTriIdxs.size() == triCount) return;

    KDTreeNode* leftChild = new KDTreeNode();
    KDTreeNode* rightChild = new KDTreeNode();
    leftChild->triIndices = leftTriIdxs;
    rightChild->triIndices = rightTriIdxs;
    node->left = leftChild;
    node->right = rightChild;
    nodesUsed++; nodesUsed++;

    node->splitAxis = axis;
    node->splitDistance = distance;

    // update the bounds of nodes
    leftChild->aabbMin = node->aabbMin;
    leftChild->aabbMax = node->aabbMax;
    leftChild->aabbMax[axis] = splitPos;

    rightChild->aabbMin = node->aabbMin;
    rightChild->aabbMax = node->aabbMax;
    rightChild->aabbMin[axis] = splitPos;

    node->triIndices.clear();
    node->isLeaf = false;

    // recurse
    Subdivide(leftChild, depth + 1);
    Subdivide(rightChild, depth + 1);
}

bool KDTree::IntersectAABB(const Ray& ray, const float3 bmin, const float3 bmax, float& tminOut, float& tmaxOut)
{
    float tx1 = (bmin.x - ray.O.x) * ray.rD.x, tx2 = (bmax.x - ray.O.x) * ray.rD.x;
    float tmin = min(tx1, tx2), tmax = max(tx1, tx2);
    float ty1 = (bmin.y - ray.O.y) * ray.rD.y, ty2 = (bmax.y - ray.O.y) * ray.rD.y;
    tmin = max(tmin, min(ty1, ty2)), tmax = min(tmax, max(ty1, ty2));
    float tz1 = (bmin.z - ray.O.z) * ray.rD.z, tz2 = (bmax.z - ray.O.z) * ray.rD.z;
    tmin = max(tmin, min(tz1, tz2)), tmax = min(tmax, max(tz1, tz2));
    tminOut = tmin;
    tmaxOut = tmax;
    return tmax >= tmin && tmin < ray.t && tmax > 0;
}

void KDTree::IntersectTri(Ray& ray, const Tri& tri, const uint triIdx)
{
    const float3 edge1 = tri.vertex1 - tri.vertex0;
    const float3 edge2 = tri.vertex2 - tri.vertex0;
    const float3 h = cross(ray.D, edge2);
    const float a = dot(edge1, h);
    if (a > -0.0001f && a < 0.0001f) return; // ray parallel to triangle
    const float f = 1 / a;
    const float3 s = ray.O - tri.vertex0;
    const float u = f * dot(s, h);
    if (u < 0 || u > 1) return;
    const float3 q = cross(s, edge1);
    const float v = f * dot(ray.D, q);
    if (v < 0 || u + v > 1) return;
    const float t = f * dot(edge2, q);
    if (t > 0.0001f)
    {
        if (t < ray.t) ray.t = min(ray.t, t), ray.objIdx = tri.objIdx, ray.triIdx = triIdx, ray.barycentric = float2(u, v);
    }
}

void KDTree::IntersectKDTree(Ray& ray, KDTreeNode* node)
{
    float tmin, tmax;
    if (node == nullptr) return;
    if (!IntersectAABB(ray, node->aabbMin, node->aabbMax, tmin, tmax)) return;
    ray.traversed++;
    if (node->isLeaf)
    {
        uint triCount = node->triIndices.size();
        for (uint i = 0; i < triCount; i++)
        {
            uint triIdx = node->triIndices[i];
            IntersectTri(ray, triangles[triIdx], triIdx);
        }
        return;
    }

    float t = (node->splitDistance - ray.O[node->splitAxis]) / ray.D[node->splitAxis];

    IntersectKDTree(ray, node->left);
    IntersectKDTree(ray, node->right);

    //if(ray.D[node->splitAxis] > 0)
    //{
    //    // t <= tmin, only the right node is intersected
    //    if (t < tmin + 0.001)
    //    {
    //        IntersectKDTree(ray, node->right);
    //    }
    //    // t >= tmax, only the left node is intersected
    //    else if (t > tmax + 0.001)
    //    {
    //        IntersectKDTree(ray, node->left);
    //    }
    //    else
    //    {
    //        IntersectKDTree(ray, node->left);
    //        IntersectKDTree(ray, node->right);
    //    }
    //}
    //else
    //{
    //    // t <= tmin, only the left node is intersected
    //    if (t < tmin + 0.001)
    //    {
    //        IntersectKDTree(ray, node->left);
    //    }
    //    // t >= tmax, only the left node is intersected
    //    else if (t > tmax + 0.001)
    //    {
    //        IntersectKDTree(ray, node->right);
    //    }
    //    else
    //    {
    //        IntersectKDTree(ray, node->right);
    //        IntersectKDTree(ray, node->left);
    //    }
    //}
}

int KDTree::GetTriangleCount() const
{
    return triangles.size();
}

void KDTree::SetTransform(mat4 transform)
{
    T = transform;
    invT = transform.FastInvertedTransformNoScale();
    // update bvh bound
    // calculate world-space bounds using the new matrix
    float3 bmin = rootNode->aabbMin, bmax = rootNode->aabbMax;
    worldBounds = aabb();
    for (int i = 0; i < 8; i++)
        worldBounds.Grow(TransformPosition(float3(i & 1 ? bmax.x : bmin.x,
            i & 2 ? bmax.y : bmin.y, i & 4 ? bmax.z : bmin.z), transform));
}

void KDTree::Intersect(Ray& ray)
{
    Ray tRay = Ray(ray);
    tRay.O = TransformPosition_SSE(ray.O4, invT);
    tRay.D = TransformVector_SSE(ray.D4, invT);
    tRay.rD = float3(1 / tRay.D.x, 1 / tRay.D.y, 1 / tRay.D.z);

    IntersectKDTree(tRay, rootNode);

    tRay.O = ray.O;
    tRay.D = ray.D;
    tRay.rD = ray.rD;
    ray = tRay;
}

float3 KDTree::GetNormal(const uint triIdx, const float2 barycentric) const
{
    float3 n0 = triangles[triIdx].normal0;
    float3 n1 = triangles[triIdx].normal1;
    float3 n2 = triangles[triIdx].normal2;
    float3 N = (1 - barycentric.x - barycentric.y) * n0 + barycentric.x * n1 + barycentric.y * n2;
    return normalize(TransformVector(N, T));
}

float2 KDTree::GetUV(const uint triIdx, const float2 barycentric) const
{
    float2 uv0 = triangles[triIdx].uv0;
    float2 uv1 = triangles[triIdx].uv1;
    float2 uv2 = triangles[triIdx].uv2;
    return (1 - barycentric.x - barycentric.y) * uv0 + barycentric.x * uv1 + barycentric.y * uv2;
}