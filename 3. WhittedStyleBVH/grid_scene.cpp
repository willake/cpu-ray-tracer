#include "precomp.h"
#include "grid_scene.h"
#include "material.h"
#include "model.h"
#include "texture.h"

GridScene::GridScene()
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
	mat4 t = mat4::Translate(float3(1, -0.4f, 0)) * mat4::Scale(0.3f);
	spaceShip = Model(1, "../assets/cube.obj", t);
	//spaceShip.material.textureDiffuse = std::make_unique<Texture>("../assets/textures/Defuse_wok.png");
	spaceShip.AppendTriangles(sceneGrid.triangles);
	printf("Triangle count: %d\n", sceneGrid.GetTriangleCount());
	sceneGrid.BuildGrid();
	skydome = Texture("../assets/industrial_sunset_puresky_4k.hdr");
	/*models[0] = Model(0, "../assets/cube.obj", mat4::Scale(0.3f));
	models[1] = Model(1, "../assets/cube.obj", mat4::Translate(0.5f, 0, 2) * mat4::Scale(0.3f));*/
	SetTime(0);
}

void GridScene::SetTime(float t)
{
	animTime = t;
	// sphere animation: bounce
	float tm = 1 - sqrf(fmodf(animTime, 2.0f) - 1);
	sphere.pos = float3(-1.8f, 0.4f + tm, 1);
}

float3 GridScene::GetSkyColor(const Ray& ray) const
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

float3 GridScene::GetLightPos() const
{
	return float3(0, 2, 0);
}

float3 GridScene::RandomPointOnLight(const float r0, const float r1) const
{
	return float3(0);
}

float3 GridScene::RandomPointOnLight(uint& seed) const
{
	return float3(0);
}

void GridScene::GetLightQuad(float3& v0, float3& v1, float3& v2, float3& v3, const uint idx = 0)
{
}

float3 GridScene::GetLightColor() const
{
	return float3(24, 24, 22);
}

float3 GridScene::GetAreaLightColor() const
{
	return float3(0);
}

float GridScene::GetLightArea() const
{
	return 0;
}

constexpr float GridScene::GetLightCount() const
{
	return 1;
}

void GridScene::FindNearest(Ray& ray)
{
	/*for (int i = 0; i < NUM_CUBE; i++)
	{
		models[i].Intersect(ray);
	}*/
	floor.Intersect(ray);
	sphere.Intersect(ray);
	sceneGrid.Intersect(ray);
}

bool GridScene::IsOccluded(const Ray& ray)
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
	sceneGrid.Intersect(shadow);
	if (shadow.objIdx > -1) return true;
	// skip planes
	return false;
}

HitInfo GridScene::GetHitInfo(const float3 I, const float2 barycentric, const int objIdx, const int triIdx)
{
	if (objIdx == 100)
	{
		return HitInfo(floor.GetNormal(I), float2(0), &materials[objIdx - 100]);
	}
	if (objIdx == 101)
	{
		return HitInfo(sphere.GetNormal(I), float2(0), &materials[objIdx - 100]);
	}
	return HitInfo(
		sceneGrid.GetNormal(triIdx, barycentric),
		sceneGrid.GetUV(triIdx, barycentric),
		objIdx == 1 ? spaceShip.GetMaterial() : &errorMaterial
	);
}

float3 GridScene::GetNormal(const float3 I, const float2 barycentric, const int objIdx, const int triIdx) const
{
	//return models[objIdx].GetNormal(triIdx);
	if (objIdx == 100) return floor.GetNormal(I);
	if (objIdx == 101) return sphere.GetNormal(I);
	return sceneGrid.GetNormal(triIdx, barycentric);
	//return float3(0);
}

float2 GridScene::GetUV(const float3 I, const float2 barycentric, const int objIdx, const int triIdx) const
{
	//return models[objIdx].GetNormal(triIdx);
	if (objIdx == 100) return float2(0);
	if (objIdx == 101) return float2(0);
	return sceneGrid.GetUV(triIdx, barycentric);
	//return float3(0);
}

float3 GridScene::GetAlbedo(int objIdx, float3 I) const
{
	if (objIdx == 100) return floor.GetAlbedo(I);
	return float3(0);
}

Material* GridScene::GetMaterial(int objIdx)
{
	if (objIdx == 1) return spaceShip.GetMaterial();
	if (objIdx > 99) return &materials[objIdx - 100];
	return &errorMaterial;
}

int GridScene::GetTriangleCount() const
{
	return sceneGrid.GetTriangleCount();
}