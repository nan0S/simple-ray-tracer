#pragma once

#include <glm/glm.hpp>

struct Ray
{
   glm::vec3 o;
   glm::vec3 d;
};

struct BarycentricTriangle
{
   glm::vec3 P;
   glm::vec3 u;
   glm::vec3 v;
};

struct Triangle
{
   union
   {
      BarycentricTriangle bar;
      glm::vec3 p[3];
   };
};

int rayTriangleIntersection(const Ray &ray, const Triangle &tri, float *t);
