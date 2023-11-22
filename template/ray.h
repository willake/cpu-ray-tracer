#pragma once

#define SPEEDTRIX

namespace Tmpl8 {
	__declspec(align(64)) class Ray
	{
	public:
		Ray() = default;
		Ray(const float3 origin, const float3 direction, const float distance = 1e34f, const int idx = -1)
		{
			O = origin, D = direction, t = distance;
			// calculate reciprocal ray direction for triangles and AABBs
			rD = float3(1 / D.x, 1 / D.y, 1 / D.z);
#ifdef SPEEDTRIX
			d0 = 1, d1 = d2 = 0; // ready for SIMD matrix math
#endif
			objIdx = idx;
		}
		float3 IntersectionPoint() const { return O + t * D; }
		// ray data
#ifndef SPEEDTRIX
		float3 O, D, rD;
#else
		union { struct { float3 O; float d0; }; __m128 O4; };
		union { struct { float3 D; float d1; }; __m128 D4; };
		union { struct { float3 rD; float d2; }; __m128 rD4; };
#endif
		float t = 1e34f;
		int objIdx = -1;
		bool inside = false; // true when in medium
	};
}