#pragma once

#include "base_scene.h"
#include "bvh.h"
#include "grid.h"
#include "kdtree.h"
#include "tlas_bvh.h"
#include "tlas_grid.h"
#include "tlas_kdtree.h"
#include "rapidxml.hpp"

//#define USE_BVH
#define USE_Grid
//#define USE_KDTree

namespace Tmpl8
{
	struct ObjectData {
		std::string modelLocation;
		std::string textureLocation;
		float3 position;
		float3 rotation;
		float3 scale;
	};

	// Define a structure to hold scene information
	struct SceneData {
		std::string name;
		float3 lightPos;
		std::string skydomeLocation;
		std::vector<ObjectData> objects;
	};


	class FileScene : BaseScene
	{
	public:
		FileScene(const string& filePath);
		SceneData LoadSceneFile(const string& filePath);
		void SetTime(float t);
		float3 GetSkyColor(const Ray& ray) const;
		float3 GetLightPos() const;
		float3 GetLightColor() const;
		void FindNearest(Ray& ray);
		bool IsOccluded(const Ray& ray);
		float3 GetAlbedo(int objIdx, float3 I) const;
		HitInfo GetHitInfo(const Ray& ray, const float3 I);
		int GetTriangleCount() const;
		std::chrono::microseconds GetBuildTime() const;
		uint GetMaxTreeDepth() const;
	public:
		float animTime = 0;
#ifdef USE_BVH
		TLASBVH tlas;
#endif
#ifdef USE_Grid
		TLASGrid tlas;
#endif
#ifdef USE_KDTree
		TLASKDTree tlas;
#endif
		string sceneName;
		Texture skydome;
		Plane floor;
		Sphere sphere;
		Quad light;
		int objIdUsed = 2;
		int objCount = 0;
		Material errorMaterial;
		Material materials[3];
	};
}
