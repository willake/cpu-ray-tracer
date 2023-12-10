#pragma once

#include "base_scene.h"
#include "tlas_bvh.h"
#include "tlas_grid.h"
#include "tlas_kdtree.h"
#include "rapidxml.hpp"

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
	public:
		float animTime = 0;
#ifdef USE_BVH
		BVH bvhs[3];
#endif
#ifdef USE_Grid
		Grid grids[3];
#endif
#ifdef USE_KDTree
		KDTree kdTrees[3];
#endif
		string sceneName;
		TLASBVH tlasBVH;
		TLASGrid tlasGrid;
		TLASKDTree tlasKDTree;
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
