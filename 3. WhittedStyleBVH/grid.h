#pragma once

// reference: https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-acceleration-structure/grid.html

namespace Tmpl8
{
	struct GridCell
	{
		std::vector<int> triIndices = {};
	};

	class Grid
	{
	private:
		aabb CalculateBounds(Tri& tri);
		void UpdateGridBounds(Tri& tri);
		bool IntersectAABB(const Ray& ray, const float3 bmin, const float3 bmax);
		bool IntersectTri(Ray& ray, const Tri& tri, const uint triIdx);
		void GetRayCellIndicies(const Ray& ray, int& minX, int& minY, int& minZ, int& maxX, int& maxY, int& maxZ);
	public:
		void BuildGrid(int3 resolution);
		void Intersect(Ray& ray);
		float3 GetNormal(const uint triIdx, const float2 barycentric) const;
		float2 GetUV(const uint triIdx, const float2 barycentric) const;
		int GetTriangleCounts() const;
		int3 resolution;
		float3 cellSize;
		aabb gridBounds;
		std::vector<Tri> triangles;
		std::vector<float3> normals;
		std::vector<uint> triangleIndices;
		std::vector<GridCell> gridCells;
	};
}