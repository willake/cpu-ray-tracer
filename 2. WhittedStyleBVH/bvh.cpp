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
    root.leftFirst = 0;
    root.triCount = triangles.size();
    UpdateNodeBounds(rootNodeIdx);
    // subdivide recursively
    Subdivide(rootNodeIdx);
}

void BVH::UpdateNodeBounds(uint nodeIdx)
{
    BVHNode& node = bvhNodes[nodeIdx];
    node.aabbMin = float3(1e30f);
    node.aabbMax = float3(-1e30f);
    for (uint first = node.leftFirst, i = 0; i < node.triCount; i++)
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

#ifdef SAH
    // determine split axis using SAH
    int bestAxis = -1;
    float bestPos = 0, bestCost = 1e30f;
    for (int axis = 0; axis < 3; axis++) for (uint i = 0; i < node.triCount; i++)
    {
        Tri& triangle = triangles[triangleIndices[node.leftFirst + i]];
        float candidatePos = triangle.centroid[axis];
        float cost = EvaluateSAH(node, axis, candidatePos);
        if (cost < bestCost)
            bestPos = candidatePos, bestAxis = axis, bestCost = cost;
    }
    int axis = bestAxis;
    float splitPos = bestPos;

    float3 e = node.aabbMax - node.aabbMin; // extent of parent
    float parentArea = e.x * e.y + e.y * e.z + e.z * e.x;
    float parentCost = node.triCount * parentArea;

    if (bestCost >= parentCost) return;
#else
    // split plane axis and position
    float3 extent = node.aabbMax - node.aabbMin;
    int axis = 0;
    if (extent.y > extent.x) axis = 1;
    if (extent.z > extent[axis]) axis = 2;
    float splitPos = node.aabbMin[axis] + extent[axis] * 0.5f;
#endif
    // split the group in two halves
    int i = node.leftFirst;
    int j = i + node.triCount - 1;
    while (i <= j)
    {
        if (triangles[triangleIndices[i]].centroid[axis] < splitPos)
            i++;
        else
            swap(triangleIndices[i], triangleIndices[j--]);
    }

    // creating child nodes for each half
    int leftCount = i - node.leftFirst;
    if (leftCount == 0 || leftCount == node.triCount) return;
    // create child nodes
    int leftChildIdx = nodesUsed++;
    int rightChildIdx = nodesUsed++;

    bvhNodes[leftChildIdx].leftFirst = node.leftFirst;
    bvhNodes[leftChildIdx].triCount = leftCount;
    bvhNodes[rightChildIdx].leftFirst = i;
    bvhNodes[rightChildIdx].triCount = node.triCount - leftCount;
    node.leftFirst = leftChildIdx;
    node.triCount = 0;
    UpdateNodeBounds(leftChildIdx);
    UpdateNodeBounds(rightChildIdx);
    // recurse
    Subdivide(leftChildIdx);
    Subdivide(rightChildIdx);
}

float BVH::EvaluateSAH(BVHNode& node, int axis, float pos)
{
    // determine triangle counts and bounds for this split candidate
    aabb leftBox, rightBox;
    int leftCount = 0, rightCount = 0;
    for (uint i = 0; i < node.triCount; i++)
    {
        Tri& triangle = triangles[triangleIndices[node.leftFirst + i]];
        if (triangle.centroid[axis] < pos)
        {
            leftCount++;
            leftBox.Grow(triangle.vertex0);
            leftBox.Grow(triangle.vertex1);
            leftBox.Grow(triangle.vertex2);
        }
        else
        {
            rightCount++;
            rightBox.Grow(triangle.vertex0);
            rightBox.Grow(triangle.vertex1);
            rightBox.Grow(triangle.vertex2);
        }
    }
    float cost = leftCount * leftBox.Area() + rightCount * rightBox.Area();
    return cost > 0 ? cost : 1e30f;
}

#ifdef FASTER_RAY
float BVH::IntersectAABB(const Ray& ray, const float3 bmin, const float3 bmax)
{
    float tx1 = (bmin.x - ray.O.x) * ray.rD.x, tx2 = (bmax.x - ray.O.x) * ray.rD.x;
    float tmin = min(tx1, tx2), tmax = max(tx1, tx2);
    float ty1 = (bmin.y - ray.O.y) * ray.rD.y, ty2 = (bmax.y - ray.O.y) * ray.rD.y;
    tmin = max(tmin, min(ty1, ty2)), tmax = min(tmax, max(ty1, ty2));
    float tz1 = (bmin.z - ray.O.z) * ray.rD.z, tz2 = (bmax.z - ray.O.z) * ray.rD.z;
    tmin = max(tmin, min(tz1, tz2)), tmax = min(tmax, max(tz1, tz2));
    if (tmax >= tmin && tmin < ray.t && tmax > 0) return tmin; else return 1e30f;
}
#else
bool BVH::IntersectAABB(const Ray& ray, const float3 bmin, const float3 bmax)
{
    float tx1 = (bmin.x - ray.O.x) * ray.rD.x, tx2 = (bmax.x - ray.O.x) * ray.rD.x;
    float tmin = min(tx1, tx2), tmax = max(tx1, tx2);
    float ty1 = (bmin.y - ray.O.y) * ray.rD.y, ty2 = (bmax.y - ray.O.y) * ray.rD.y;
    tmin = max(tmin, min(ty1, ty2)), tmax = min(tmax, max(ty1, ty2));
    float tz1 = (bmin.z - ray.O.z) * ray.rD.z, tz2 = (bmax.z - ray.O.z) * ray.rD.z;
    tmin = max(tmin, min(tz1, tz2)), tmax = min(tmax, max(tz1, tz2));
    return tmax >= tmin && tmin < ray.t && tmax > 0;
}
#endif
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
#ifdef FASTER_RAY
    BVHNode* node = &bvhNodes[rootNodeIdx], * stack[64];
    uint stackPtr = 0;
    while (1)
    {
        if (node->isLeaf())
        {
            for (uint i = 0; i < node->triCount; i++)
            {
                uint triIdx = triangleIndices[node->leftFirst + i];
                IntersectTri(ray, triangles[triIdx], triIdx);
            }
            if (stackPtr == 0) break; else node = stack[--stackPtr];

            continue;
        }
        BVHNode* child1 = &bvhNodes[node->leftFirst];
        BVHNode* child2 = &bvhNodes[node->leftFirst + 1];
        float dist1 = IntersectAABB(ray, child1->aabbMin, child1->aabbMax);
        float dist2 = IntersectAABB(ray, child2->aabbMin, child2->aabbMax);
        if (dist1 > dist2) { swap(dist1, dist2); swap(child1, child2); }
        if (dist1 == 1e30f)
        {
            if (stackPtr == 0) break; else node = stack[--stackPtr];
        }
        else
        {
            node = child1;
            if (dist2 != 1e30f) stack[stackPtr++] = child2;
        }
    }
#else
    BVHNode& node = bvhNodes[nodeIdx];
    if (!IntersectAABB(ray, node.aabbMin, node.aabbMax)) return;
    if (node.isLeaf())
    {
        for (uint i = 0; i < node.triCount; i++)
        {
            uint triIdx = triangleIndices[node.leftFirst + i];
            IntersectTri(ray, triangles[triIdx], triIdx);
        }
    }
    else
    {
        IntersectBVH(ray, node.leftFirst);
        IntersectBVH(ray, node.leftFirst + 1);
    }
#endif
}

int BVH::GetTriangleCount() const
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

void BVH::IntersectTriDebug(Ray& ray, const Tri& tri, const uint triIdx)
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

    if ((u > 0.03f && u < 1 - 0.03f) && (v > 0.03f || v < 1 - 0.03f)) return;

    if (t > 0.0001f)
    {
        if (t < ray.t) ray.t = min(ray.t, t), ray.objIdx = tri.objIdx, ray.triIdx = triIdx, ray.barycentric = float2(u, v);
    }
}

void BVH::IntersectBVHDebug(Ray& ray, const uint nodeIdx)
{
#ifdef FASTER_RAY
    BVHNode* node = &bvhNodes[rootNodeIdx], * stack[64];
    uint stackPtr = 0;
    while (1)
    {
        if (node->isLeaf())
        {
            for (uint i = 0; i < node->triCount; i++)
            {
                uint triIdx = triangleIndices[node->leftFirst + i];
                IntersectTriDebug(ray, triangles[triIdx], triIdx);
            }
            if (stackPtr == 0) break; else node = stack[--stackPtr];

            continue;
        }
        BVHNode* child1 = &bvhNodes[node->leftFirst];
        BVHNode* child2 = &bvhNodes[node->leftFirst + 1];
        float dist1 = IntersectAABB(ray, child1->aabbMin, child1->aabbMax);
        float dist2 = IntersectAABB(ray, child2->aabbMin, child2->aabbMax);
        if (dist1 > dist2) { swap(dist1, dist2); swap(child1, child2); }
        if (dist1 == 1e30f)
        {
            if (stackPtr == 0) break; else node = stack[--stackPtr];
        }
        else
        {
            node = child1;
            if (dist2 != 1e30f) stack[stackPtr++] = child2;
        }
    }
#else
    BVHNode& node = bvhNodes[nodeIdx];
    if (!IntersectAABB(ray, node.aabbMin, node.aabbMax)) return;
    if (node.isLeaf())
    {
        for (uint i = 0; i < node.triCount; i++)
        {
            uint triIdx = triangleIndices[node.leftFirst + i];
            IntersectTri(ray, triangles[triIdx], triIdx);
        }
    }
    else
    {
        IntersectBVH(ray, node.leftFirst);
        IntersectBVH(ray, node.leftFirst + 1);
    }
#endif
}

void BVH::IntersectDebug(Ray& ray)
{
    IntersectBVHDebug(ray, rootNodeIdx);
}