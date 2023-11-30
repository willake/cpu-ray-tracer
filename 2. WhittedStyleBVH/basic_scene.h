#pragma once

//#define SPEEDTRIX

namespace Tmpl8
{
	class BasicScene
	{
	public:
		BasicScene();
		void SetTime(float t);
		float3 GetSkyColor(const Ray& ray) const;
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
		HitInfo GetHitInfo(const Ray& ray, const float3 I);
		float3 GetAbsorption(int objIdx);
		int GetTriangleCount() const;
	public:
		__declspec(align(64)) // start a new cacheline here
			float animTime = 0;
		Quad quad, dummyQuad1, dummyQuad2, dummyQuad3;
		Sphere sphere;
		Sphere sphere2;
		Cube cube;
		Plane plane[6];
		Torus torus;
		Material materials[11];
		Material errorMaterial = Material(MaterialType::Diffuse, float3(255, 192, 203) / 255.f);
	};
}
