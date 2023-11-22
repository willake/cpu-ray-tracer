#pragma once

#define SPEEDTRIX

namespace Tmpl8
{
	class BasicScene
	{
	public:
		BasicScene();
		void SetTime(float t);
		float3 GetLightPos() const;
		float3 RandomPointOnLight(const float r0, const float r1) const;
		float3 RandomPointOnLight(uint& seed) const;
		void GetLightQuad(float3& v0, float3& v1, float3& v2, float3& v3, const uint idx);
		float3 GetLightColor() const;
		float3 GetAreaLightColor() const;
		float GetLightArea() const;
		constexpr float GetLightCount() const;
		void FindNearest(Ray& ray) const;
		bool IsOccluded(const Ray& ray) const;
		float3 GetNormal(const int objIdx, const float3 I, const float3 wo) const;
		float3 GetAlbedo(int objIdx, float3 I) const;
		Material* GetMaterial(int objIdx);
		float GetReflectivity(int objIdx, float3 I) const;
		float GetRefractivity(int objIdx, float3 I) const;
		float3 GetAbsorption(int objIdx);
	public:
		__declspec(align(64)) // start a new cacheline here
			float animTime = 0;
#ifdef FOURLIGHTS
		Quad quad[4];
#else
		Quad quad, dummyQuad1, dummyQuad2, dummyQuad3;
#endif
		Sphere sphere;
		Sphere sphere2;
		Cube cube;
		Plane plane[6];
		Torus torus;
		Material materials[11];
	};
}
