#pragma once

namespace Tmpl8
{
	struct BVHNode
	{
		float3 aabbMin, aabbMax;     // 24 bytes
		uint leftChild, rightChild;  // 8 bytes
		bool isLeaf;                 // 4 bytes
		uint firstPrim, primCount;   // 8 bytes; total: 44 bytes
	};
}