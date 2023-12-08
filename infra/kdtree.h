#pragma once

#define SAH
#define FASTER_RAY
// reference: https://jacco.ompf2.com/2022/04/13/how-to-build-a-bvh-part-1-basics/

namespace Tmpl8
{
	struct KDTreeNode
	{
		float3 aabbMin = 0, aabbMax = 0;
		KDTreeNode* left;
		KDTreeNode* right;
		int splitAxis = 0;
		float splitDistance = 0;
		std::vector<uint> triIndices;
		bool isLeaf = true;
	};

	class KDTree
	{
	private:
		void UpdateBounds();
		void Subdivide(KDTreeNode* node, int depth);
		bool IntersectAABB(const Ray& ray, const float3 bmin, const float3 bmax, float& tmin, float& tmax);
		void IntersectTri(Ray& ray, const Tri& tri, const uint triIdx);
		void IntersectKDTree(Ray& ray, KDTreeNode* node);
	public:
		KDTree() = default;
		KDTree(const int idx, const std::string& modelPath, const mat4 transform, const mat4 scaleMat);
		void Build();
		void Intersect(Ray& ray);
		void SetTransform(mat4 transform);
		float3 GetNormal(const uint triIdx, const float2 barycentric) const;
		float2 GetUV(const uint triIdx, const float2 barycentric) const;
		int GetTriangleCount() const;
	private:
		int m_maxBuildDepth = 20;
	public:
		int objIdx = -1;
		KDTreeNode* rootNode;
		std::vector<Tri> triangles;
		std::vector<aabb> triangleBounds;
		std::vector<uint> triangleIndices;
		uint rootNodeIdx = 0, nodesUsed = 1;
		Material material;
		aabb localBounds;
		aabb worldBounds;
		mat4 T, invT;
	};
}