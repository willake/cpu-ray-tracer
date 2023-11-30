#include "precomp.h"
#include "grid_scene.h"
#include "material.h"
#include "model.h"
#include "texture.h"

GridScene::GridScene()
{
	light = Quad(0, 1);
	floor = Plane(1, float3(0, 1, 0), 1);
	sphere = Sphere(2, float3(0), 0.6f);
	materials[0] = Material(MaterialType::Light);
	materials[1] = Material(MaterialType::Diffuse, float3(0), true);
	materials[2] = Material(MaterialType::Mirror);
	mat4 t = mat4::Translate(float3(1, -0.4f, 0)) * mat4::Scale(0.5);
	wok = Model(3, "../assets/wok.obj", t);
	wok.material.textureDiffuse = std::make_unique<Texture>("../assets/textures/Defuse_wok.png");
	wok.AppendTriangles(sceneGrid.triangles);
	printf("Triangle count: %d\n", sceneGrid.GetTriangleCount());
	sceneGrid.BuildGrid();
	skydome = Texture("../assets/industrial_sunset_puresky_4k.hdr");
	SetTime(0);
}

void GridScene::SetTime(float t)
{
	animTime = t;
	// sphere animation: bounce
	float tm = 1 - sqrf(fmodf(animTime, 2.0f) - 1);
	sphere.pos = float3(-1.8f, 0.4f + tm, 1);

	// light source animation: swing
	mat4 M1base = mat4::Translate(float3(0, 2.6f, 2));
	mat4 M1 = M1base * mat4::RotateZ(sinf(animTime * 0.6f) * 0.1f) * mat4::Translate(float3(0, -0.9f, 0));
	light.T = M1, light.invT = M1.FastInvertedTransformNoScale();
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
	light.Intersect(ray);
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

HitInfo GridScene::GetHitInfo(const Ray& ray, const float3 I)
{
	HitInfo hitInfo = HitInfo(float3(0), float2(0), &errorMaterial);
	switch (ray.objIdx)
	{
	case 0:
		hitInfo.normal = light.GetNormal(I);
		hitInfo.uv = float2(0);
		hitInfo.material = &materials[0];
		break;
	case 1:
		hitInfo.normal = floor.GetNormal(I);
		hitInfo.uv = float2(0);
		hitInfo.material = &materials[1];
		break;
	case 2:
		hitInfo.normal = sphere.GetNormal(I);
		hitInfo.uv = float2(0);
		hitInfo.material = &materials[2];
		break;
	case 3:
		hitInfo.normal = sceneGrid.GetNormal(ray.triIdx, ray.barycentric);
		hitInfo.uv = sceneGrid.GetUV(ray.triIdx, ray.barycentric);
		hitInfo.material = &wok.material;
		break;
	default:
		break;
	}
	return hitInfo;
}

float3 GridScene::GetAlbedo(int objIdx, float3 I) const
{
	if (objIdx == 1) return floor.GetAlbedo(I);
	return float3(0);
}

int GridScene::GetTriangleCount() const
{
	return sceneGrid.GetTriangleCount();
}