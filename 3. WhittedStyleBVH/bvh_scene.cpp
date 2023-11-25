#include "precomp.h"
#include "bvh_scene.h"
#include "material.h"
#include "model.h"
#include "texture.h"

BVHScene::BVHScene()
{
	floor = Plane(100, float3(0, 1, 0), 1);
	sphere = Sphere(101, float3(0), 0.6f);
	materials[0] = Material(MaterialType::Diffuse, float3(0), true);
	materials[1] = Material(MaterialType::Mirror);
	/*or (int i = 0; i < NUM_CUBE; i++)
	{
		float3 rpos(RandomFloat(), RandomFloat(), RandomFloat());
		float3 rscale(RandomFloat() * 0.3f);
		mat4 t = mat4::Translate(rpos * 3 - float3(1.5f)) * mat4::Scale(rscale);
		models[i] = Model(i, "../assets/cube.obj", t);
		models[i].AppendTriangles(sceneBVH.triangles);
	}*/
	mat4 t = mat4::Translate(float3(2, -0.4f, 0)) * mat4::Scale(1.f);
	spaceShip = Model(1, "../assets/bunny.obj", t);
	spaceShip.AppendTriangles(sceneBVH.triangles);
	printf("Triangle count: %d", sceneBVH.GetTriangleCounts());
	sceneBVH.BuildBVH();
	skydome = Texture("../assets/industrial_sunset_puresky_4k.hdr");
	/*models[0] = Model(0, "../assets/cube.obj", mat4::Scale(0.3f));
	models[1] = Model(1, "../assets/cube.obj", mat4::Translate(0.5f, 0, 2) * mat4::Scale(0.3f));*/
	SetTime(0);
}

void BVHScene::SetTime(float t)
{
	animTime = t;
	// sphere animation: bounce
	float tm = 1 - sqrf(fmodf(animTime, 2.0f) - 1);
	sphere.pos = float3(-1.8f, 0.4f + tm, 1);
}

float3 BVHScene::GetSkyColor(const Ray& ray) const
{
	// Convert ray direction to texture coordinates
	float theta = acos(ray.D.y);  // Assuming a spherical skydome
	float phi = atan2(ray.D.z, ray.D.x);

	// Normalize to[0, 1]
	float u = phi / (2 * PI);
	float v = 1.0f - (theta / PI);

	// Sample the HDR skydome texture
	float3 color = skydome.Sample(u, v);

	return color;
}

float3 BVHScene::GetLightPos() const
{
	return float3(0, 2, 0);
}

float3 BVHScene::RandomPointOnLight(const float r0, const float r1) const
{
	return float3(0);
}

float3 BVHScene::RandomPointOnLight(uint& seed) const
{
	return float3(0);
}

void BVHScene::GetLightQuad(float3& v0, float3& v1, float3& v2, float3& v3, const uint idx = 0)
{
}

float3 BVHScene::GetLightColor() const
{
	return float3(24, 24, 22);
}

float3 BVHScene::GetAreaLightColor() const
{
	return float3(0);
}

float BVHScene::GetLightArea() const
{
	return 0;
}

constexpr float BVHScene::GetLightCount() const
{
	return 1;
}

void BVHScene::FindNearest(Ray& ray)
{
	/*for (int i = 0; i < NUM_CUBE; i++)
	{
		models[i].Intersect(ray);
	}*/
	floor.Intersect(ray);
	sphere.Intersect(ray);
	sceneBVH.Intersect(ray);
}

bool BVHScene::IsOccluded(const Ray& ray)
{
	/*for (int i = 0; i < NUM_CUBE; i++)
	{
		if (models[i].IsOccluded(ray))
		{
			return true;
		}
	}*/
	// from tmpl8rt_IGAD
	if (sphere.IsOccluded(ray)) return true;
	Ray shadow = ray;
	shadow.t = 1e34f;
	sceneBVH.Intersect(shadow);
	if (shadow.objIdx > -1) return true;
	// skip planes
	return false;
}

float3 BVHScene::GetNormal(const float3 I, const float2 barycentric, const int objIdx, const int triIdx) const
{
	//return models[objIdx].GetNormal(triIdx);
	if (objIdx == 100) return floor.GetNormal(I);
	if (objIdx == 101) return sphere.GetNormal(I);
	return sceneBVH.GetNormal(triIdx, barycentric);
	//return float3(0);
}

float3 BVHScene::GetAlbedo(int objIdx, float3 I) const 
{ 
	if (objIdx == 100) return floor.GetAlbedo(I);
	return float3(0); 
}

Material* BVHScene::GetMaterial(int objIdx)
{
	if (objIdx == 1) return spaceShip.GetMaterial();
	if (objIdx > 99) return &materials[objIdx - 100];
	return &errorMaterial;
}