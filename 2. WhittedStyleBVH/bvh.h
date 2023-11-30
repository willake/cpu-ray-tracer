#pragma once

#define SAH
#define FASTER_RAY

// reference: https://jacco.ompf2.com/2022/04/13/how-to-build-a-bvh-part-1-basics/

namespace Tmpl8
{
	struct BVHNode
	{
		float3 aabbMin, aabbMax;     // 24 bytes
		uint leftFirst, triCount;   // 8 bytes; total: 32 bytes
		// If it is 0, leftFirst contains the index of the left child node.
		// Otherwise, it contains the index of the first triangle index.
		bool isLeaf() { return triCount > 0; }
	};

	class BVH
	{
	private:
		void UpdateNodeBounds(uint nodeIdx);
		void Subdivide(uint nodeIdx);
#ifdef FASTER_RAY
		float IntersectAABB(const Ray& ray, const float3 bmin, const float3 bmax);
#else
		bool IntersectAABB(const Ray& ray, const float3 bmin, const float3 bmax);
#endif
		void IntersectTri(Ray& ray, const Tri& tri, const uint triIdx);
		void IntersectBVH(Ray& ray, const uint nodeIdx);
		void IntersectTriDebug(Ray& ray, const Tri& tri, const uint triIdx);
		void IntersectBVHDebug(Ray& ray, const uint nodeIdx);
		float EvaluateSAH(BVHNode& node, int axis, float pos);
	public:
		void BuildBVH();
		void Intersect(Ray& ray);
		void IntersectDebug(Ray& ray);
		float3 GetNormal(const uint triIdx, const float2 barycentric) const;
		float2 GetUV(const uint triIdx, const float2 barycentric) const;
		int GetTriangleCount() const;
		std::vector<Tri> triangles;
		std::vector<float3> normals;
		std::vector<uint> triangleIndices;
		std::vector<BVHNode> bvhNodes;
		uint rootNodeIdx = 0, nodesUsed = 1;
	};
}