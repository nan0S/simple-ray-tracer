#pragma once

#include <vector>

#include "Math.h"

using uint = unsigned int;

struct MeshData
{
   glm::vec3 ka;
   glm::vec3 kd;
   glm::vec3 ks;
};

struct Light
{
   glm::vec3 position;
   glm::vec3 color;
   float intensity;
};

struct RayTracerData
{
   std::vector<Triangle> tris;
   std::vector<glm::vec3> normals;
   std::vector<uint> mesh_indices;
   std::vector<MeshData> mesh_data;
   std::vector<Light> lights;
};

void rayTrace(RayTracerData *rtdata, int xres, int yres, float focal_length,
              glm::vec3 origin, glm::vec3 forward, glm::vec3 right,
              float dist_bound, int k, glm::vec3 *output);
glm::vec3 rayTrace(Ray ray, RayTracerData *rtdata, float inv_dist_bound, int depth);
