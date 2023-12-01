#pragma once

// reference: https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-acceleration-structure/grid.html
// reference: https://cs184.eecs.berkeley.edu/sp19/lecture/9-44/raytracing

namespace Tmpl8
{
	struct GridCell
	{
		std::vector<int> triIndices = {};
	};

	class Grid
	{
	private:
		bool IntersectAABB(const Ray& ray, const float3 bmin, const float3 bmax);
		bool IntersectTri(Ray& ray, const Tri& tri, const uint triIdx);
	public:
		void Build();
		void Intersect(Ray& ray);
		float3 GetNormal(const uint triIdx, const float2 barycentric) const;
		float2 GetUV(const uint triIdx, const float2 barycentric) const;
		int GetTriangleCount() const;
		int3 resolution;
		float3 cellSize;
		aabb gridBounds;
		std::vector<Tri> triangles;
		std::vector<float3> normals;
		std::vector<uint> triangleIndices;
		std::vector<GridCell> gridCells;
	};
}