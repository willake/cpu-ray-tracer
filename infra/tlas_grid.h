#pragma once

namespace Tmpl8
{
    struct TLASGridNode
    {
        float3 aabbMin = float3(0);
        uint leftRight = 0; // 2x16 bits
        float3 aabbMax = float3(0);
        uint BLAS = 0;
        bool isLeaf() { return leftRight == 0; }
    };

    struct TLASGridCell
    {
        std::vector<int> blasIndices = {};
    };

    class TLASGrid
    {
    private:
        bool IntersectAABB(const Ray& ray, const float3 bmin, const float3 bmax);
        void IntersectGrid(Ray& ray, long uid);
        int FindBestMatch(int* list, int N, int A);
    public:
        TLASGrid() = default;
        TLASGrid(std::vector<Grid*> blasList);
        void Build();
        void Intersect(Ray& ray);
    private:
        int3 resolution = 0;
        float3 cellSize = 0;
        TLASGridNode* tlasNode;
        uint nodesUsed = 0, blasCount;
        aabb localBounds;
        std::vector<long> mailbox;
        long incremental = 0;
    public:
        std::vector<Grid*> blas;
        std::vector<TLASGridCell> gridCells;
        std::chrono::microseconds buildTime;
    };
}
