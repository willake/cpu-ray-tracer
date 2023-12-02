#pragma once

#define SAH
#define FASTER_RAY
#define BINS 8

// reference: https://jacco.ompf2.com/2022/04/13/how-to-build-a-bvh-part-1-basics/

namespace Tmpl8
{
	struct Bin { aabb bounds; int triCount = 0; };

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
		float EvaluateSAH(BVHNode& node, int axis, float pos);
		float FindBestSplitPlane(BVHNode& node, int& axis, float& splitPos);
		float CalculateNodeCost(BVHNode& node);
	public:
		BVH() = default;
		BVH(const int idx, const std::string& modelPath, const mat4 transform, const mat4 scaleMat);
		void Build();
		void Refit();
		void Intersect(Ray& ray);
		void SetTransform(mat4 transform);
		float3 GetNormal(const uint triIdx, const float2 barycentric) const;
		float2 GetUV(const uint triIdx, const float2 barycentric) const;
		int GetTriangleCount() const;
	private:
		std::vector<Tri> triangles;
		std::vector<uint> triangleIndices;
		uint rootNodeIdx = 0, nodesUsed = 1;
	public:
		int objIdx = -1;
		std::vector<BVHNode> bvhNodes;
		Material material;
		aabb bounds;
		mat4 T, invT;
	};
}