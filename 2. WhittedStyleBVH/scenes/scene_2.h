#pragma once

#include "base_scene.h"
#include "tlas_bvh.h"

namespace Tmpl8
{
	class Scene2 : BaseScene
	{
	public:
		Scene2();
		void SetTime(float t);
		float3 GetSkyColor(const Ray& ray) const;
		float3 GetLightPos() const;
		float3 GetLightColor() const;
		void FindNearest(Ray& ray);
		bool IsOccluded(const Ray& ray);
		float3 GetAlbedo(int objIdx, float3 I) const;
		HitInfo GetHitInfo(const Ray& ray, const float3 I);
		int GetTriangleCount() const;
	public:
		float animTime = 0;
		BVH bvhs[3];
		std::vector<Grid> grids;
		TLASBVH tlasBVH;
		Texture skydome;
		Plane floor;
		Sphere sphere;
		Quad light;
		Material errorMaterial = Material(MaterialType::Diffuse, float3(255, 192, 203) / 255.f);
		Material materials[3];
	};
}
