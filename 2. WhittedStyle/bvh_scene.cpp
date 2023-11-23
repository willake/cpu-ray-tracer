#include "precomp.h"

BVHScene::BVHScene()
{
	for (int i = 0; i < NUM_CUBE; i++)
	{
		float3 rpos(RandomFloat(), RandomFloat(), RandomFloat());
		float3 rscale(RandomFloat() * 0.5);
		mat4 t = mat4::Translate(rpos * 3 - float3(1.5)) * mat4::Scale(rscale);
		models[i] = Model(i, "../assets/cube.obj", t);
	}
	/*models[0] = Model(0, "../assets/cube.obj", mat4::Scale(0.3f));
	models[1] = Model(1, "../assets/cube.obj", mat4::Translate(0.5f, 0, 2) * mat4::Scale(0.3f));*/
}

void BVHScene::SetTime(float t)
{

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

void BVHScene::IntersectTri(Ray& ray, const Tri& tri, const int idx) const
{
	const float3 edge1 = tri.vertex1 - tri.vertex0;
	const float3 edge2 = tri.vertex2 - tri.vertex0;
	const float3 h = cross(ray.D, edge2);
	const float a = dot(edge1, h);
	if (a > -0.0001f && a < 0.0001f) return; // ray parallel to triangle
	const float f = 1 / a;
	const float3 s = ray.O - tri.vertex0;
	const float u = f * dot(s, h);
	if (u < 0 || u > 1) return;
	const float3 q = cross(s, edge1);
	const float v = f * dot(ray.D, q);
	if (v < 0 || u + v > 1) return;
	const float t = f * dot(edge2, q);
	if (t > 0.0001f)
	{
		if(t < ray.t) ray.t = min(ray.t, t), ray.objIdx = idx;
	}
}

void BVHScene::FindNearest(Ray& ray) const
{
	/*for (int i = 0; i < NUM_TRI; i++)
	{
		IntersectTri(ray, tri[i], i);
	}*/
	for (int i = 0; i < NUM_CUBE; i++)
	{
		models[i].Intersect(ray);
	}
}

bool BVHScene::IsOccluded(const Ray& ray) const
{
	for (int i = 0; i < NUM_CUBE; i++)
	{
		if (models[i].IsOccluded(ray))
		{
			return true;
		}
	}
	return false;
}

float3 BVHScene::GetNormal(const int objIdx, const int triIdx) const
{
	return models[objIdx].GetNormal(triIdx);
	//return float3(0);
}

float3 BVHScene::GetAlbedo(int objIdx, float3 I) const { return float3(0); }

Material* BVHScene::GetMaterial(int objIdx)
{
	return models[objIdx].GetMaterial();
}

float BVHScene::GetReflectivity(int objIdx, float3 I) const
{
	return 0;
}

float BVHScene::GetRefractivity(int objIdx, float3 I) const
{
	return 0;
}

float3 BVHScene::GetAbsorption(int objIdx)
{
	return float3(0);
}