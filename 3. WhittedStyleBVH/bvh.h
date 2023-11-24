#pragma once

namespace Tmpl8
{
	struct BVHNode
	{
		float3 aabbMin, aabbMax;     // 24 bytes
		uint leftNode;  // 4 bytes
		uint firstTriIdx, triCount;   // 8 bytes; total: 36 bytes
		bool isLeaf() { return triCount > 0; }
	};

	class BVH
	{
	private:
		void UpdateNodeBounds(uint nodeIdx);
		void Subdivide(uint nodeIdx);
		bool IntersectAABB(const Ray& ray, const float3 bmin, const float3 bmax);
		void IntersectTri(Ray& ray, const Tri& tri, const uint triIdx);
		void IntersectBVH(Ray& ray, const uint nodeIdx);
	public:
		void BuildBVH();
		void Intersect(Ray& ray);
		float3 GetNormal(const uint triIdx) const;
		std::vector<Tri> triangles;
		std::vector<float3> normals;
		std::vector<uint> triangleIndices;
		std::vector<BVHNode> bvhNodes;
		uint rootNodeIdx = 0, nodesUsed = 1;
	};
}