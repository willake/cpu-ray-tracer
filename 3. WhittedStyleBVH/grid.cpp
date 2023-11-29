#include "precomp.h"
#include "grid.h"

void Grid::BuildGrid(int3 res)
{
    resolution = res;
    gridCells.resize(resolution.x * resolution.y * resolution.z);

    // Determine scene bound
    for (size_t i = 0; i < triangles.size(); i++)
    {
        UpdateGridBounds(triangles[i]);
    }

    float3 gridArea = gridBounds.bmax3 - gridBounds.bmin3;
    cellSize = float3(gridArea.x / resolution.x, gridArea.y / resolution.y, gridArea.z / resolution.z);

    // Put triangles into grids
    for (size_t triIdx = 0; triIdx < triangles.size(); triIdx++)
    {
        aabb bounds = CalculateBounds(triangles[triIdx]);

        // Determine grid cell range for the object
        int minX = clamp(static_cast<int>((bounds.bmin3.x - gridBounds.bmin3.x) / gridArea.x * resolution.x), 0, resolution.x - 1);
        int minY = clamp(static_cast<int>((bounds.bmin3.y - gridBounds.bmin3.y) / gridArea.y * resolution.y), 0, resolution.y - 1);
        int minZ = clamp(static_cast<int>((bounds.bmin3.z - gridBounds.bmin3.z) / gridArea.z * resolution.z), 0, resolution.z - 1);
        int maxX = clamp(static_cast<int>((bounds.bmax3.x - gridBounds.bmin3.x) / gridArea.x * resolution.x), 0, resolution.x - 1);
        int maxY = clamp(static_cast<int>((bounds.bmax3.y - gridBounds.bmin3.y) / gridArea.y * resolution.y), 0, resolution.y - 1);
        int maxZ = clamp(static_cast<int>((bounds.bmax3.z - gridBounds.bmin3.z) / gridArea.z * resolution.z), 0, resolution.z - 1);

        // Assign the object to the corresponding grid cells
        for (int iz = minZ; iz <= maxZ; ++iz) {
            for (int iy = minY; iy <= maxY; ++iy) {
                for (int ix = minX; ix <= maxX; ++ix) {
                    int cellIndex = ix + iy * resolution.x + iz * resolution.x * resolution.y;
                    gridCells[cellIndex].triIndices.push_back(triIdx);
                }
            }
        }
    }
}

void Grid::UpdateGridBounds(Tri& tri)
{
    gridBounds.Grow(tri.vertex0);
    gridBounds.Grow(tri.vertex1);
    gridBounds.Grow(tri.vertex2);
}

aabb Grid::CalculateBounds(Tri& tri)
{
    aabb bounds;
    bounds.Grow(tri.vertex0);
    bounds.Grow(tri.vertex1);
    bounds.Grow(tri.vertex2);
    return bounds;
}

bool Grid::IntersectAABB(const Ray& ray, const float3 bmin, const float3 bmax)
{
    float tx1 = (bmin.x - ray.O.x) * ray.rD.x, tx2 = (bmax.x - ray.O.x) * ray.rD.x;
    float tmin = min(tx1, tx2), tmax = max(tx1, tx2);
    float ty1 = (bmin.y - ray.O.y) * ray.rD.y, ty2 = (bmax.y - ray.O.y) * ray.rD.y;
    tmin = max(tmin, min(ty1, ty2)), tmax = min(tmax, max(ty1, ty2));
    float tz1 = (bmin.z - ray.O.z) * ray.rD.z, tz2 = (bmax.z - ray.O.z) * ray.rD.z;
    tmin = max(tmin, min(tz1, tz2)), tmax = min(tmax, max(tz1, tz2));
    return tmax >= tmin && tmin < ray.t && tmax > 0;
}

bool Grid::IntersectTri(Ray& ray, const Tri& tri, const uint triIdx)
{
    const float3 edge1 = tri.vertex1 - tri.vertex0;
    const float3 edge2 = tri.vertex2 - tri.vertex0;
    const float3 h = cross(ray.D, edge2);
    const float a = dot(edge1, h);
    if (a > -0.0001f && a < 0.0001f) return false; // ray parallel to triangle
    const float f = 1 / a;
    const float3 s = ray.O - tri.vertex0;
    const float u = f * dot(s, h);
    if (u < 0 || u > 1) return false;
    const float3 q = cross(s, edge1);
    const float v = f * dot(ray.D, q);
    if (v < 0 || u + v > 1) return false;
    const float t = f * dot(edge2, q);
    if (t > 0.0001f)
    {
        if (t < ray.t) ray.t = min(ray.t, t), ray.objIdx = tri.objIdx, ray.triIdx = triIdx, ray.barycentric = float2(u, v);
        return true;
    }
    return false;
}

int Grid::GetTriangleCounts() const
{
    return triangles.size();
}

void Grid::GetRayCellIndicies(const Ray& ray, int& minX, int& minY, int& minZ, int& maxX, int& maxY, int& maxZ)
{
    // Calculate the minimum and maximum cell indices that the ray traverses
    float tMinX = (gridBounds.bmin3.x - ray.O.x) / ray.D.x;
    float tMaxX = (gridBounds.bmax3.x - ray.O.x) / ray.D.x;
    if (ray.D.x < 0.0f) swap(tMinX, tMaxX);

    float tMinY = (gridBounds.bmin3.y - ray.O.y) / ray.D.y;
    float tMaxY = (gridBounds.bmax3.y - ray.O.y) / ray.D.y;
    if (ray.D.y < 0.0f) swap(tMinY, tMaxY);
    
    float tMinZ = (gridBounds.bmin3.z - ray.O.z) / ray.D.z;
    float tMaxZ = (gridBounds.bmax3.z - ray.O.z) / ray.D.z;
    if (ray.D.z < 0.0f) swap(tMinZ, tMaxZ);

    minX = clamp(static_cast<int>(tMinX), 0, resolution.x - 1);
    minY = clamp(static_cast<int>(tMinY), 0, resolution.y - 1);
    minZ = clamp(static_cast<int>(tMinZ), 0, resolution.z - 1);
    maxX = clamp(static_cast<int>(tMaxX), 0, resolution.x - 1);
    maxY = clamp(static_cast<int>(tMaxY), 0, resolution.y - 1);
    maxZ = clamp(static_cast<int>(tMaxZ), 0, resolution.z - 1);
}

void Grid::Intersect(Ray& ray)
{
    // Calculate tmin and tmax
    float tmin = 0, tmax = 1e34f;
    for (int i = 0; i < 3; ++i) {
        float t0 = (gridBounds.bmin3[i] - ray.O[i]) * ray.rD[i];
        float t1 = (gridBounds.bmax3[i] - ray.O[i]) * ray.rD[i];

        if (ray.rD[i] < 0.0f) {
            std::swap(t0, t1);
        }

        tmin = std::max(t0, tmin);
        tmax = std::min(t1, tmax);

        if (tmin > tmax) {
            // No intersection with the bounding box
            return;
        }
    }
    
    // Determine the cell indices that the ray traverses
    int3 exit, step, cell;
    float3 deltaT, nextCrossingT;
    for (int i = 0; i < 3; i++)
    {
        float rayOrigCell = (ray.O[i] + ray.D[i] * tmin) - gridBounds.bmin3[i];
        cell[i] = clamp(static_cast<int>(std::floor(rayOrigCell / cellSize[i])), 0, resolution[i] - 1);
        if (ray.D[i] < 0)
        {
            deltaT[i] = -cellSize[i] * ray.rD[i];
            nextCrossingT[i] = tmin + (cell[i] * cellSize[i] - rayOrigCell) * ray.rD[i];
            exit[i] = -1;
            step[i] = -1;
        }
        else
        {
            deltaT[i] = cellSize[i] * ray.rD[i];
            nextCrossingT[i] = tmin + ((cell[i] + 1) * cellSize[i] - rayOrigCell) * ray.rD[i];
            exit[i] = resolution[i];
            step[i] = 1;
        }
    }
    bool isIntersect = false;
    while (!isIntersect)
    {
        uint index = cell.z * resolution.x * resolution.y + cell.y * resolution.x + cell.x;
        for (int triIdx : gridCells[index].triIndices)
        {
            if (IntersectTri(ray, triangles[triIdx], triIdx))
            {
                isIntersect = true;
            }
        }

        uint k =
            ((nextCrossingT.x < nextCrossingT.y) << 2) +
            ((nextCrossingT.x < nextCrossingT.z) << 1) +
            ((nextCrossingT.y < nextCrossingT.z));
        static const uint8_t map[8] = { 2, 1, 2, 1, 2, 2, 0, 0 };
        uint8_t axis = map[k];
        if (ray.t < nextCrossingT[axis]) break;
        cell[axis] += step[axis];
        if (cell[axis] == exit[axis]) break;
        nextCrossingT[axis] += deltaT[axis];
    }
}

float3 Grid::GetNormal(const uint triIdx, const float2 barycentric) const
{
    float3 n0 = triangles[triIdx].normal0;
    float3 n1 = triangles[triIdx].normal1;
    float3 n2 = triangles[triIdx].normal2;
    return (1 - barycentric.x - barycentric.y) * n0 + barycentric.x * n1 + barycentric.y * n2;
    //return normals[triIdx];
}

float2 Grid::GetUV(const uint triIdx, const float2 barycentric) const
{
    float2 uv0 = triangles[triIdx].uv0;
    float2 uv1 = triangles[triIdx].uv1;
    float2 uv2 = triangles[triIdx].uv2;
    return (1 - barycentric.x - barycentric.y) * uv0 + barycentric.x * uv1 + barycentric.y * uv2;
}