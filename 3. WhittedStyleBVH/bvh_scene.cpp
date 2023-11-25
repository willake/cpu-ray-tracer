#include "precomp.h"

BVHScene::BVHScene()
{
	sphere = Sphere(100, float3(0), 0.6f);
	materialSphere = Material(MaterialType::Glass);
	for (int i = 0; i < NUM_CUBE; i++)
	{
		float3 rpos(RandomFloat(), RandomFloat(), RandomFloat());
		float3 rscale(RandomFloat() * 0.3);
		mat4 t = mat4::Translate(rpos * 3 - float3(1.5)) * mat4::Scale(rscale);
		models[i] = Model(i, "../assets/cube.obj", t);
		models[i].AppendTriangles(sceneBVH.triangles);
	}
	models[0].material = Material(MaterialType::Glass);
	models[2].material = Material(MaterialType::Mirror);

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
	sphere.pos = float3(-1.8f, -0.4f + tm, 1);
}

float3 BVHScene::GetSkyColor(const Ray& ray) const
{
	// Convert ray direction to texture coordinates
	float theta = acos(ray.D.y);  // Assuming a spherical skydome
	float phi = atan2(ray.D.z, ray.D.x);

	// Normalize to[0, 1]
	float u = phi / (2 * PI);
	float v = 1.0 - (theta / PI);

	// Sample the HDR skydome texture
	float3 color = skydome.Sample(u, v);

	return color;
}

float3 BVHScene::GetLightPos() const
{
	return float3(2, 1, -1);
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
	sphere.Intersect(ray);
	//sceneBVH.Intersect(ray);
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
	//sceneBVH.Intersect(shadow);
	if (shadow.objIdx > -1) return true;
	return false;
}

float3 BVHScene::GetNormal(const float3 I, const int objIdx, const int triIdx) const
{
	//return models[objIdx].GetNormal(triIdx);
	if (objIdx == 100) return sphere.GetNormal(I);
	return sceneBVH.GetNormal(triIdx);
	//return float3(0);
}

float3 BVHScene::GetAlbedo(int objIdx, float3 I) const { return float3(0); }

Material* BVHScene::GetMaterial(int objIdx)
{
	if (objIdx == 100) return &materialSphere;
	return models[objIdx].GetMaterial();
}