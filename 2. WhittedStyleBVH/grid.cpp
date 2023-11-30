#include "precomp.h"
#include "grid.h"

void Grid::BuildGrid()
{
    // Determine scene bound
    for (size_t i = 0; i < triangles.size(); i++)
    {
        gridBounds.Grow(triangles[i].GetBounds());
    }

    float3 gridSize = gridBounds.bmax3 - gridBounds.bmin3;

    // dynamically calculate resolution
    float cubeRoot = powf(5 * GetTriangleCount() / (gridSize.x * gridSize.y * gridSize.z), 1 / 3.f);
    for (int i = 0; i < 3; i++)
    {
        resolution[i] = static_cast<int>(floor(gridSize[i] * cubeRoot));
        resolution[i] = max(1, min(resolution[i], 128));
    }

    gridCells.resize(resolution.x * resolution.y * resolution.z);

    cellSize = float3(gridSize.x / resolution.x, gridSize.y / resolution.y, gridSize.z / resolution.z);

    // Put triangles into grids
    for (size_t triIdx = 0; triIdx < triangles.size(); triIdx++)
    {
        aabb bounds = triangles[triIdx].GetBounds();

        // Determine grid cell range for the object
        int minX = clamp(static_cast<int>((bounds.bmin3.x - gridBounds.bmin3.x) / cellSize.x), 0, resolution.x - 1);
        int minY = clamp(static_cast<int>((bounds.bmin3.y - gridBounds.bmin3.y) / cellSize.y), 0, resolution.y - 1);
        int minZ = clamp(static_cast<int>((bounds.bmin3.z - gridBounds.bmin3.z) / cellSize.z), 0, resolution.z - 1);
        int maxX = clamp(static_cast<int>((bounds.bmax3.x - gridBounds.bmin3.x) / cellSize.x), 0, resolution.x - 1);
        int maxY = clamp(static_cast<int>((bounds.bmax3.y - gridBounds.bmin3.y) / cellSize.y), 0, resolution.y - 1);
        int maxZ = clamp(static_cast<int>((bounds.bmax3.z - gridBounds.bmin3.z) / cellSize.z), 0, resolution.z - 1);

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

int Grid::GetTriangleCount() const
{
    return triangles.size();
}

void Grid::Intersect(Ray& ray)
{
    // Calculate tmin and tmax
    if (!IntersectAABB(ray, gridBounds.bmin3, gridBounds.bmax3)) return;

    // Determine the cell indices that the ray traverses
    int3 exit, step, cell;
    float3 deltaT, nextCrossingT;
    for (int i = 0; i < 3; ++i)
    {
        float rayOrigCell = ray.O[i] - gridBounds.bmin3[i];
        cell[i] = clamp(static_cast<int>(std::floor(rayOrigCell / cellSize[i])), 0, resolution[i] - 1);
        if (ray.D[i] < 0)
        {
            deltaT[i] = -cellSize[i] * ray.rD[i];
            nextCrossingT[i] = (cell[i] * cellSize[i] - rayOrigCell) * ray.rD[i];
            exit[i] = -1;
            step[i] = -1;
        }
        else
        {
            deltaT[i] = cellSize[i] * ray.rD[i];
            nextCrossingT[i] = ((cell[i] + 1) * cellSize[i] - rayOrigCell) * ray.rD[i];
            exit[i] = resolution[i];
            step[i] = 1;
        }
    }

    while (true)
    {
        uint index = cell.x + cell.y * resolution.x + cell.z * resolution.x * resolution.y;
        for (int triIdx : gridCells[index].triIndices)
        {
            IntersectTri(ray, triangles[triIdx], triIdx);
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