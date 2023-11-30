#pragma once

#define NUM_CUBE 10
#include "model.h"
#include "texture.h"
#include "material.h"
#include "grid.h"

namespace Tmpl8
{
	// -----------------------------------------------------------
	// Scene class
	// We intersect this. The query is internally forwarded to the
	// list of primitives, so that the nearest hit can be returned.
	// For this hit (distance, obj id), we can query the normal and
	// albedo.
	// -----------------------------------------------------------
	class GridScene
	{
	public:
		GridScene();
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
		void FindNearest(Ray& ray);
		bool IsOccluded(const Ray& ray);
		float3 GetAlbedo(int objIdx, float3 I) const;
		HitInfo GetHitInfo(const float3 I, const float2 barycentric, const int objIdx, const int triIdx);
		int GetTriangleCount() const;
	public:
		float animTime = 0;
		Model wok;
		//Model models[NUM_CUBE];
		Texture skydome;
		Grid sceneGrid;
		Quad light;
		Plane floor;
		Sphere sphere;
		Material errorMaterial = Material(MaterialType::Diffuse, float3(255, 192, 203) / 255.f);
		Material materials[3];
	};
}
