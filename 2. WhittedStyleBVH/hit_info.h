#pragma once

struct HitInfo
{
public:
	HitInfo() = default;
	HitInfo(float3 n, float2 u, Material* mat) : normal(n), uv(u), material(mat) {}
	float3 normal;
	float2 uv;
	Material* material;
};