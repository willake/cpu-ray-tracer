#include "precomp.h"
#include "tlas_grid.h"

TLASGrid::TLASGrid(std::vector<Grid*> blasList)
{
	blasCount = blasList.size();
	// copy a pointer to the array of bottom level accstructs
	for (int i = 0; i < blasCount; i++)
	{
		blas.push_back(blasList[i]);
	}
	// allocate TLAS nodes
	tlasNode = (TLASGridNode*)_aligned_malloc(sizeof(TLASGridNode) * 2 * blasCount, 64);
	Build();
}

void TLASGrid::Build()
{
	auto startTime = std::chrono::high_resolution_clock::now();
	for (size_t i = 0; i < blas.size(); i++)
	{
		localBounds.Grow(blas[i]->worldBounds);
	}

	float3 gridSize = localBounds.bmax3 - localBounds.bmin3;

	// dynamically calculate resolution
	float cubeRoot = powf(5 * blas.size() / (gridSize.x * gridSize.y * gridSize.z), 1 / 3.f);
	for (int i = 0; i < 3; i++)
	{
		resolution[i] = static_cast<int>(floor(gridSize[i] * cubeRoot));
		resolution[i] = max(1, min(resolution[i], 128));
	}

	gridCells.resize(resolution.x * resolution.y * resolution.z);

	cellSize = float3(gridSize.x / resolution.x, gridSize.y / resolution.y, gridSize.z / resolution.z);

	// Put triangles into grids
	for (size_t blasIdx = 0; blasIdx < blas.size(); blasIdx++)
	{
		aabb bounds = blas[blasIdx]->worldBounds;

		// Determine grid cell range for the object
		int minX = clamp(static_cast<int>((bounds.bmin3.x - localBounds.bmin3.x) / cellSize.x), 0, resolution.x - 1);
		int minY = clamp(static_cast<int>((bounds.bmin3.y - localBounds.bmin3.y) / cellSize.y), 0, resolution.y - 1);
		int minZ = clamp(static_cast<int>((bounds.bmin3.z - localBounds.bmin3.z) / cellSize.z), 0, resolution.z - 1);
		int maxX = clamp(static_cast<int>((bounds.bmax3.x - localBounds.bmin3.x) / cellSize.x), 0, resolution.x - 1);
		int maxY = clamp(static_cast<int>((bounds.bmax3.y - localBounds.bmin3.y) / cellSize.y), 0, resolution.y - 1);
		int maxZ = clamp(static_cast<int>((bounds.bmax3.z - localBounds.bmin3.z) / cellSize.z), 0, resolution.z - 1);

		// Assign the object to the corresponding grid cells
		for (int iz = minZ; iz <= maxZ; ++iz) {
			for (int iy = minY; iy <= maxY; ++iy) {
				for (int ix = minX; ix <= maxX; ++ix) {
					int cellIndex = ix + iy * resolution.x + iz * resolution.x * resolution.y;
					gridCells[cellIndex].blasIndices.push_back(blasIdx);
				}
			}
		}
	}
	auto endTime = std::chrono::high_resolution_clock::now();
	buildTime = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime);
}

int TLASGrid::FindBestMatch(int* list, int N, int A)
{
	float smallest = 1e30f;
	int bestB = -1;
	for (int B = 0; B < N; B++) if (B != A)
	{
		float3 bmax = fmaxf(tlasNode[list[A]].aabbMax, tlasNode[list[B]].aabbMax);
		float3 bmin = fminf(tlasNode[list[A]].aabbMin, tlasNode[list[B]].aabbMin);
		float3 e = bmax - bmin;
		float surfaceArea = e.x * e.y + e.y * e.z + e.z * e.x;
		if (surfaceArea < smallest) smallest = surfaceArea, bestB = B;
	}
	return bestB;
}

bool TLASGrid::IntersectAABB(const Ray& ray, const float3 bmin, const float3 bmax)
{
	float tx1 = (bmin.x - ray.O.x) * ray.rD.x, tx2 = (bmax.x - ray.O.x) * ray.rD.x;
	float tmin = min(tx1, tx2), tmax = max(tx1, tx2);
	float ty1 = (bmin.y - ray.O.y) * ray.rD.y, ty2 = (bmax.y - ray.O.y) * ray.rD.y;
	tmin = max(tmin, min(ty1, ty2)), tmax = min(tmax, max(ty1, ty2));
	float tz1 = (bmin.z - ray.O.z) * ray.rD.z, tz2 = (bmax.z - ray.O.z) * ray.rD.z;
	tmin = max(tmin, min(tz1, tz2)), tmax = min(tmax, max(tz1, tz2));
	return tmax >= tmin && tmin < ray.t && tmax > 0;
}

void TLASGrid::IntersectGrid(Ray& ray, long uid)
{
	ray.tested++;
	ray.traversed++;
	// Calculate tmin and tmax
	if (!IntersectAABB(ray, localBounds.bmin3, localBounds.bmax3)) return;

	// Determine the cell indices that the ray traverses
	int3 exit, step, cell;
	float3 deltaT, nextCrossingT;
	for (int i = 0; i < 3; ++i)
	{
		float rayOrigCell = ray.O[i] - localBounds.bmin3[i];
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
		ray.traversed++;
		uint index = cell.x + cell.y * resolution.x + cell.z * resolution.x * resolution.y;
		for (int blasIdx : gridCells[index].blasIndices)
		{
			ray.tested++;
			if (IntersectAABB(ray, blas[blasIdx]->worldBounds.bmin3, blas[blasIdx]->worldBounds.bmax3))
			{
				blas[blasIdx]->Intersect(ray);
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

void TLASGrid::Intersect(Ray& ray)
{
	IntersectGrid(ray, 0);
}