#pragma once

#include "base_scene.h"
#include "bvh.h"
#include "grid.h"

namespace Tmpl8
{
	class AnimableScene : BaseScene
	{
	public:
		AnimableScene();
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
		Model tower;
		//Model models[NUM_CUBE];
		Texture skydome;
		BVH sceneBVH;
		Plane floor;
		Sphere sphere;
		Quad light;
		Material errorMaterial = Material(MaterialType::Diffuse, float3(255, 192, 203) / 255.f);
		Material materials[3];
	};
}
