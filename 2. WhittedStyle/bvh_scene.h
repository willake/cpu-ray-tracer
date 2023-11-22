#pragma once

#define NUM_TRI 64

namespace Tmpl8
{
	struct Tri { float3 vertex0, vertex1, vertex2; float3 centroid; };

	// -----------------------------------------------------------
	// Scene class
	// We intersect this. The query is internally forwarded to the
	// list of primitives, so that the nearest hit can be returned.
	// For this hit (distance, obj id), we can query the normal and
	// albedo.
	// -----------------------------------------------------------
	class BVHScene
	{
	public:
		BVHScene();
		void SetTime(float t);
		float3 GetLightPos() const;
		float3 RandomPointOnLight(const float r0, const float r1) const;
		float3 RandomPointOnLight(uint& seed) const;
		void GetLightQuad(float3& v0, float3& v1, float3& v2, float3& v3, const uint idx);
		float3 GetLightColor() const;
		float3 GetAreaLightColor() const;
		float GetLightArea() const;
		constexpr float GetLightCount() const;
		void IntersectTri(Ray& ray, const Tri& tri, const int idx) const;
		void FindNearest(Ray& ray) const;
		bool IsOccluded(const Ray& ray) const;
		float3 GetNormal(const int objIdx, const float3 I, const float3 wo) const;
		float3 GetAlbedo(int objIdx, float3 I) const;
		Material* GetMaterial(int objIdx);
		float GetReflectivity(int objIdx, float3 I) const;
		float GetRefractivity(int objIdx, float3 I) const;
		float3 GetAbsorption(int objIdx);
	public:
		float animTime = 0;
		Tri tri[NUM_TRI];
		Material material;
	};
}
