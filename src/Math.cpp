#include "Math.h"

#define EPSILON 0.000001
// #define TEST_CULL

int rayTriangleIntersection(const Ray &ray, const Triangle &tri, float *t)
{
   glm::vec3 pvec = glm::cross(ray.d, tri.bar.v);
   float det = glm::dot(tri.bar.u, pvec);
#ifdef TEST_CULL
   if (det < EPSILON)
      return 0;
   glm::vec3 tvec = ray.o - tri.bar.P;
   float u = glm::dot(tvec, pvec);
   if (u < 0 || u > det)
      return 0;
   glm::vec3 qvec = glm::cross(tvec, tri.bar.u);
   float v = glm::dot(ray.d, qvec);
   if (v < 0 || u + v > det)
      return 0;
   *t = glm::dot(tri.bar.v, qvec);
   *t /= det;
#else
   if (det > -EPSILON && det < EPSILON)
     return 0;
   float inv_det = 1 / det;
   glm::vec3 tvec = ray.o - tri.bar.P;
   float u = glm::dot(tvec, pvec) * inv_det;
   if (u < 0 || u > 1)
     return 0;
   glm::vec3 qvec = glm::cross(tvec, tri.bar.u);
   float v = glm::dot(ray.d, qvec) * inv_det;
   if (v < 0 || u + v > 1)
     return 0;
   *t = glm::dot(tri.bar.v, qvec) * inv_det;
#endif
   return 1;
}
