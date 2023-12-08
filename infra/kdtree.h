#pragma once

//#define SAH
#define FASTER_RAY
#define BINS 8
// reference: https://www.youtube.com/watch?v=TrqK-atFfWY&ab_channel=JustinSolomon
// reference: https://github.com/reddeupenn/kdtreePathTracerOptimization
// refernece: On building fast kd-Trees for Ray Tracing, and on doing that in O(N log N) by Ingo Wald and Vlastimil Havran

namespace Tmpl8
{
	struct KDTreeNode
	{
		float3 aabbMin = 0, aabbMax = 0; // 24 bytes
		uint left; // 4 bytes
		uint splitAxis = 0; // 4 bytes
		float splitDistance = 0; // 4 bytes
		bool isLeaf = true; // 2 bytes    total 38 bytes
		std::vector<uint> triIndices;
	};

	class KDTree
	{
	private:
		void UpdateBounds();
		float CalculateNodeCost(KDTreeNode& node);
		float FindBestSplitPlane(KDTreeNode& node, int& axis, float& splitPos);
		void Subdivide(uint nodeIdx, int depth);
		float EvaluateSAH(KDTreeNode& node, int axis, float pos);
		bool IntersectAABB(const Ray& ray, const float3 bmin, const float3 bmax, float& tmin, float& tmax);
		void IntersectTri(Ray& ray, const Tri& tri, const uint triIdx);
		void IntersectKDTree(Ray& ray, uint nodeIdx);
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
		int m_maxBuildDepth = 100;
	public:
		int objIdx = -1;
		std::vector<KDTreeNode> nodes;
		std::vector<Tri> triangles;
		std::vector<aabb> triangleBounds;
		uint rootNodeIdx = 0, nodesUsed = 1;
		Material material;
		aabb localBounds;
		aabb worldBounds;
		mat4 T, invT;
	};
}