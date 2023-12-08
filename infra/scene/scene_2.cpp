#include "precomp.h"
#include "scene_1.h"

Scene2::Scene2()
{
	errorMaterial.albedo = float3(255, 192, 203) / 255.f;
	light = Quad(0, 1);
	floor = Plane(1, float3(0, 1, 0), 1);
	sphere = Sphere(2, float3(0), 0.6f);
	materials[0].isLight = true;
	materials[1].reflectivity = 0.3f;
	materials[2].refractivity = 1.0f;
	materials[2].absorption = float3(0.5f, 0.5f, 0.5f);
	mat4 t = mat4::Translate(float3(1.5f, -0.6f, 0));
	mat4 s = mat4::Scale(0.5f);
	mat4 t2 = mat4::Translate(float3(-1.5f, -0.6f, 0));
	mat4 s2 = mat4::Scale(0.5f);
	mat4 t3 = mat4::Translate(float3(0, -0.6f, 3));
	mat4 s3 = mat4::Scale(0.5f);

	//bvhs[0] = BVH(100, "../assets/wok.obj", t, s);
	//bvhs[1] = BVH(101, "../assets/wok.obj", t2, s2);
	//bvhs[2] = BVH(102, "../assets/wok.obj", t3, s3);
	//tlasBVH = TLASBVH(bvhs, 3);
	//tlasBVH.Build();
	//tlasBVH.blas[0].material.textureDiffuse = std::make_unique<Texture>("../assets/textures/Defuse_wok.png");
	//tlasBVH.blas[1].material.textureDiffuse = std::make_unique<Texture>("../assets/textures/Defuse_wok.png");
	//tlasBVH.blas[2].material.textureDiffuse = std::make_unique<Texture>("../assets/textures/Defuse_wok.png");

	//grids[0] = Grid(200, "../assets/wok.obj", t, s);
	//grids[1] = Grid(201, "../assets/wok.obj", t2, s2);
	//grids[2] = Grid(202, "../assets/wok.obj", t3, s3);
	//tlasGrid = TLASGrid(grids, 3);
	//tlasGrid.Build();
	//tlasGrid.blas[0].material.textureDiffuse = std::make_unique<Texture>("../assets/textures/Defuse_wok.png");
	//tlasGrid.blas[1].material.textureDiffuse = std::make_unique<Texture>("../assets/textures/Defuse_wok.png");
	//tlasGrid.blas[2].material.textureDiffuse = std::make_unique<Texture>("../assets/textures/Defuse_wok.png");

	kdTrees[0] = KDTree(300, "../assets/wok.obj", t, s);
	/*kdTrees[1] = KDTree(301, "../assets/wok.obj", t2, s2);
	kdTrees[2] = KDTree(302, "../assets/wok.obj", t3, s3);*/
	tlasKDTree = TLASKDTree(kdTrees, 3);
	tlasKDTree.Build();
	tlasKDTree.blas[0].material.textureDiffuse = std::make_unique<Texture>("../assets/textures/Defuse_wok.png");
	//tlasKDTree.blas[1].material.textureDiffuse = std::make_unique<Texture>("../assets/textures/Defuse_wok.png");
	//tlasKDTree.blas[2].material.textureDiffuse = std::make_unique<Texture>("../assets/textures/Defuse_wok.png");
	skydome = Texture("../assets/industrial_sunset_puresky_4k.hdr");
	SetTime(0);
}

void Scene2::SetTime(float t)
{
	animTime = t;
	// sphere animation: bounce
	float tm = 1 - sqrf(fmodf(animTime, 2.0f) - 1);
	sphere.pos = float3(0.f, 0.5f + tm, 1.f);

	/*mat4 modelT = mat4::Translate(float3(2.f, -0.6f + tm, 1));
	bvhs[0].SetTransform(modelT);*/
	//grids[0].SetTransform(modelT);
	// light source animation: swing
	mat4 M1base = mat4::Translate(float3(0, 2.6f, 2));
	mat4 M1 = M1base * mat4::RotateZ(sinf(animTime * 0.6f) * 0.1f) * mat4::Translate(float3(0, -0.9f, 0));
	light.T = M1, light.invT = M1.FastInvertedTransformNoScale();
}

float3 Scene2::GetSkyColor(const Ray& ray) const
{
	// Convert ray direction to texture coordinates, assuming a spherical skydome
	float phi = atan2(-ray.D.z, ray.D.x) + PI;
	float theta = acos(-ray.D.y);
	float u = phi * INV2PI;
	float v = theta * INVPI;

	//// Sample the HDR skydome texture
	float3 color = skydome.Sample(u, v);

	return color;
}

float3 Scene2::GetLightPos() const
{
	return float3(0, 2, 0);
}

float3 Scene2::GetLightColor() const
{
	return float3(24, 24, 22);
}


void Scene2::FindNearest(Ray& ray)
{
	light.Intersect(ray);
	floor.Intersect(ray);
	sphere.Intersect(ray);

	//tlasBVH.Intersect(ray);
	//tlasGrid.Intersect(ray);
	tlasKDTree.Intersect(ray);
}

bool Scene2::IsOccluded(const Ray& ray)
{
	// from tmpl8rt_IGAD
	if (sphere.IsOccluded(ray)) return true;
	if (light.IsOccluded(ray)) return true;
	Ray shadow = Ray(ray);
	shadow.t = 1e34f;
	//tlasBVH.Intersect(shadow);
	//tlasGrid.Intersect(shadow);
	tlasKDTree.Intersect(shadow);
	if (shadow.objIdx > -1) return true;
	// skip planes
	return false;
}

HitInfo Scene2::GetHitInfo(const Ray& ray, const float3 I)
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
		hitInfo.normal = bvhs[0].GetNormal(ray.triIdx, ray.barycentric);
		hitInfo.uv = bvhs[0].GetUV(ray.triIdx, ray.barycentric);
		hitInfo.material = &bvhs[0].material;
		/*hitInfo.normal = gridModels[0].GetNormal(ray.triIdx, ray.barycentric);
		hitInfo.uv = gridModels[0].GetUV(ray.triIdx, ray.barycentric);
		hitInfo.material = &gridModels[0].material;*/
		break;
	case 4:
		hitInfo.normal = bvhs[1].GetNormal(ray.triIdx, ray.barycentric);
		hitInfo.uv = bvhs[1].GetUV(ray.triIdx, ray.barycentric);
		hitInfo.material = &bvhs[1].material;
		/*hitInfo.normal = gridModels[1].GetNormal(ray.triIdx, ray.barycentric);
		hitInfo.uv = gridModels[1].GetUV(ray.triIdx, ray.barycentric);
		hitInfo.material = &gridModels[1].material;*/
		break;
	default:
		break;
	}

	if (ray.objIdx > 99 && ray.objIdx < 200)
	{
		BVH& bvh = tlasBVH.blas[ray.objIdx - 100];
		hitInfo.normal = bvh.GetNormal(ray.triIdx, ray.barycentric);
		hitInfo.uv = bvh.GetUV(ray.triIdx, ray.barycentric);
		hitInfo.material = &bvh.material;
	}

	if (ray.objIdx > 199 && ray.objIdx < 300)
	{
		Grid& grid = tlasGrid.blas[ray.objIdx - 200];
		hitInfo.normal = grid.GetNormal(ray.triIdx, ray.barycentric);
		hitInfo.uv = grid.GetUV(ray.triIdx, ray.barycentric);
		hitInfo.material = &grid.material;
	}

	if (ray.objIdx > 299 && ray.objIdx < 400)
	{
		KDTree& kdtree = tlasKDTree.blas[ray.objIdx - 300];
		hitInfo.normal = kdtree.GetNormal(ray.triIdx, ray.barycentric);
		hitInfo.uv = kdtree.GetUV(ray.triIdx, ray.barycentric);
		hitInfo.material = &kdtree.material;
	}

	if (dot(hitInfo.normal, ray.D) > 0) hitInfo.normal = -hitInfo.normal;

	return hitInfo;
}

float3 Scene2::GetAlbedo(int objIdx, float3 I) const
{
	if (objIdx == 1) return floor.GetAlbedo(I);
	return float3(0);
}

int Scene2::GetTriangleCount() const
{
	int count = 0;
	/*for (int i = 0; i < 3; i++)
	{
		count += bvhs[i].GetTriangleCount();
	}*/
	//for (int i = 0; i < 3; i++)
	//{
	//	count += grids[i].GetTriangleCount();
	//}
	//for (int i = 0; i < 3; i++)
	//{
	//	count += kdTrees[i].GetTriangleCount();
	//}
	return count;
}