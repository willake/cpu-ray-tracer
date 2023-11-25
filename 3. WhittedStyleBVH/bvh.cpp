#include "precomp.h"
#include "bvh.h"

void BVH::BuildBVH()
{
    normals.resize(triangles.size());
    triangleIndices.resize(triangles.size());
    // populate triangle index array
    for (int i = 0; i < triangles.size(); i++)
    {
        // setup normals
        float3 edge1 = triangles[i].vertex1 - triangles[i].vertex0;
        float3 edge2 = triangles[i].vertex2 - triangles[i].vertex0;
        normals[i] = normalize(cross(edge1, edge2));
        // setup indices
        triangleIndices[i] = i;
    }
    // assign all triangles to root node
    bvhNodes.resize(triangles.size() * 2 - 1);
    BVHNode& root = bvhNodes[rootNodeIdx];
    root.leftNode = 0;
    root.firstTriIdx = 0, root.triCount = triangles.size();
    UpdateNodeBounds(rootNodeIdx);
    // subdivide recursively
    Subdivide(rootNodeIdx);
}

void BVH::UpdateNodeBounds(uint nodeIdx)
{
    BVHNode& node = bvhNodes[nodeIdx];
    node.aabbMin = float3(1e30f);
    node.aabbMax = float3(-1e30f);
    for (uint first = node.firstTriIdx, i = 0; i < node.triCount; i++)
    {
        uint leafTriIdx = triangleIndices[first + i];
        Tri& leafTri = triangles[leafTriIdx];
        node.aabbMin = fminf(node.aabbMin, leafTri.vertex0);
        node.aabbMin = fminf(node.aabbMin, leafTri.vertex1);
        node.aabbMin = fminf(node.aabbMin, leafTri.vertex2);
        node.aabbMax = fmaxf(node.aabbMax, leafTri.vertex0);
        node.aabbMax = fmaxf(node.aabbMax, leafTri.vertex1);
        node.aabbMax = fmaxf(node.aabbMax, leafTri.vertex2);
    }
}

void BVH::Subdivide(uint nodeIdx)
{
    // terminate recursion
    BVHNode& node = bvhNodes[nodeIdx];
    if (node.triCount <= 2) return;

    // split plane axis and position
    float3 extent = node.aabbMax - node.aabbMin;
    int axis = 0;
    if (extent.y > extent.x) axis = 1;
    if (extent.z > extent[axis]) axis = 2;
    float splitPos = node.aabbMin[axis] + extent[axis] * 0.5f;
    
    // split the group in two halves
    int i = node.firstTriIdx;
    int j = i + node.triCount - 1;
    while (i <= j)
    {
        if (triangles[triangleIndices[i]].centroid[axis] < splitPos)
            i++;
        else
            swap(triangleIndices[i], triangleIndices[j--]);
    }

    // creating child nodes for each half
    int leftCount = i - node.firstTriIdx;
    if (leftCount == 0 || leftCount == node.triCount) return;
    // create child nodes
    int leftChildIdx = nodesUsed++;
    int rightChildIdx = nodesUsed++;

    bvhNodes[leftChildIdx].firstTriIdx = node.firstTriIdx;
    bvhNodes[leftChildIdx].triCount = leftCount;
    bvhNodes[rightChildIdx].firstTriIdx = i;
    bvhNodes[rightChildIdx].triCount = node.triCount - leftCount;
    node.leftNode = leftChildIdx;
    node.triCount = 0;
    UpdateNodeBounds(leftChildIdx);
    UpdateNodeBounds(rightChildIdx);
    // recurse
    Subdivide(leftChildIdx);
    Subdivide(rightChildIdx);
}

bool BVH::IntersectAABB(const Ray& ray, const float3 bmin, const float3 bmax)
{
    float tx1 = (bmin.x - ray.O.x) / ray.D.x, tx2 = (bmax.x - ray.O.x) / ray.D.x;
    float tmin = min(tx1, tx2), tmax = max(tx1, tx2);
    float ty1 = (bmin.y - ray.O.y) / ray.D.y, ty2 = (bmax.y - ray.O.y) / ray.D.y;
    tmin = max(tmin, min(ty1, ty2)), tmax = min(tmax, max(ty1, ty2));
    float tz1 = (bmin.z - ray.O.z) / ray.D.z, tz2 = (bmax.z - ray.O.z) / ray.D.z;
    tmin = max(tmin, min(tz1, tz2)), tmax = min(tmax, max(tz1, tz2));
    return tmax >= tmin && tmin < ray.t && tmax > 0;
}

void BVH::IntersectTri(Ray& ray, const Tri& tri, const uint triIdx)
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

void BVH::IntersectBVH(Ray& ray, const uint nodeIdx)
{
    BVHNode& node = bvhNodes[nodeIdx];
    if (!IntersectAABB(ray, node.aabbMin, node.aabbMax)) return;
    if (node.isLeaf())
    {
        for (uint i = 0; i < node.triCount; i++)
        {
            uint triIdx = triangleIndices[node.firstTriIdx + i];
            IntersectTri(ray, triangles[triIdx], triIdx);
        }
    }
    else
    {
        IntersectBVH(ray, node.leftNode);
        IntersectBVH(ray, node.leftNode + 1);
    }
}

int BVH::GetTriangleCounts() const
{
    return triangles.size();
}

void BVH::Intersect(Ray& ray)
{
    IntersectBVH(ray, rootNodeIdx);
}

float3 BVH::GetNormal(const uint triIdx, const float2 barycentric) const
{
    float3 n0 = triangles[triIdx].normal0;
    float3 n1 = triangles[triIdx].normal1;
    float3 n2 = triangles[triIdx].normal2;
    return (1 - barycentric.x - barycentric.y) * n0 + barycentric.x * n1 + barycentric.y * n2;
    //return normals[triIdx];
}

float2 BVH::GetUV(const uint triIdx, const float2 barycentric) const
{
    float2 uv0 = triangles[triIdx].uv0;
    float2 uv1 = triangles[triIdx].uv1;
    float2 uv2 = triangles[triIdx].uv2;
    return (1 - barycentric.x - barycentric.y) * uv0 + barycentric.x * uv1 + barycentric.y * uv2;
}
