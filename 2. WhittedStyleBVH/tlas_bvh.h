#pragma once

namespace Tmpl8
{
    struct TLASBVHNode
    {
        float3 aabbMin;
        uint leftRight; // 2x16 bits
        float3 aabbMax;
        uint BLAS;
        bool isLeaf() { return leftRight == 0; }
    };

    class TLASBVH
    {
    public:
        TLASBVH() = default;
        TLASBVH(std::vector<BVH>& bvhList);
        void Build();
        void Intersect(Ray& ray);
    private:
        float IntersectAABB(const Ray& ray, const float3 bmin, const float3 bmax);
        int FindBestMatch(int* list, int N, int A);
        std::vector<TLASBVHNode> tlasNode;
        std::vector<BVH> blas;
        uint nodesUsed, blasCount;
    };
}