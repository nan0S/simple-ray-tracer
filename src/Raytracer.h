#pragma once

#include <glm/glm.hpp>

#include <vector>

#define EPS 0.000001

using uint = unsigned int;
using real = float;
using col3 = glm::vec3;
using vec3 = glm::vec<3, real>;

struct BarycentricTriangle
{
   vec3 P;
   vec3 u;
   vec3 v;
};

struct Triangle
{
   union
   {
      BarycentricTriangle bar;
      vec3 p[3];
   };
};

struct Material
{
   col3 ka;
   col3 kd;
   col3 ks;
};

struct Light
{
   vec3 position;
   col3 color;
   float intensity;
};

struct Ray
{
   vec3 o;
   vec3 d;
};

struct RayTracerData
{
   std::vector<Triangle> tris;
   std::vector<vec3> normals;
   std::vector<uint> mat_indices;
   std::vector<Material> materials;
   std::vector<Light> lights;
};

void rayTrace(RayTracerData *rtdata, int xres, int yres, real focal_length,
              vec3 origin, vec3 forward, vec3 right, int k, col3 *output);
