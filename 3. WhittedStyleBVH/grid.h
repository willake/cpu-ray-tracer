#pragma once

// reference: https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-acceleration-structure/grid.html

namespace Tmpl8
{
	struct GridCell
	{
		float3 aabbMin, aabbMax;     // 24 bytes
		uint leftFirst, triCount;   // 8 bytes; total: 32 bytes
		// If it is 0, leftFirst contains the index of the left child node.
		// Otherwise, it contains the index of the first triangle index.
		bool isLeaf() { return triCount > 0; }
	};

	class Grid
	{
	private:
		void UpdateNodeBounds(uint nodeIdx);
		void Subdivide(uint nodeIdx);
		bool IntersectAABB(const Ray& ray, const float3 bmin, const float3 bmax);
		void IntersectTri(Ray& ray, const Tri& tri, const uint triIdx);
		void IntersectGrid(Ray& ray, const uint nodeIdx);
	public:
		void BuildGrid();
		void Intersect(Ray& ray);
		float3 GetNormal(const uint triIdx, const float2 barycentric) const;
		float2 GetUV(const uint triIdx, const float2 barycentric) const;
		int GetTriangleCounts() const;
		std::vector<Tri> triangles;
		std::vector<float3> normals;
		std::vector<uint> triangleIndices;
		std::vector<GridCell> gridCells;
		uint rootNodeIdx = 0, nodesUsed = 1;
	};
}