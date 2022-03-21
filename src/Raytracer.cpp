#include "Raytracer.h"

#include "Utils/Timer.h"
#include "Const.h"

static col3 rayTrace(Ray ray, RayTracerData *rtdata, int depth);

// #define TEST_CULL

int rayTriangleIntersection(const Ray &ray, const Triangle &tri, real *t)
{
   vec3 pvec = glm::cross(ray.d, tri.bar.v);
   real det = glm::dot(tri.bar.u, pvec);
#ifdef TEST_CULL
   if (det < EPSILON)
      return 0;
   vec3 tvec = ray.o - tri.bar.P;
   real u = glm::dot(tvec, pvec);
   if (u < 0 || u > det)
      return 0;
   vec3 qvec = glm::cross(tvec, tri.bar.u);
   real v = glm::dot(ray.d, qvec);
   if (v < 0 || u + v > det)
      return 0;
   *t = glm::dot(tri.bar.v, qvec);
   *t /= det;
#else
   if (det > -EPS && det < EPS)
     return 0;
   real inv_det = 1 / det;
   vec3 tvec = ray.o - tri.bar.P;
   real u = glm::dot(tvec, pvec) * inv_det;
   if (u < 0 || u > 1)
     return 0;
   vec3 qvec = glm::cross(tvec, tri.bar.u);
   real v = glm::dot(ray.d, qvec) * inv_det;
   if (v < 0 || u + v > 1)
     return 0;
   *t = glm::dot(tri.bar.v, qvec) * inv_det;
#endif
   return 1;
}

void rayTrace(RayTracerData *rtdata, int xres, int yres, real focal_length,
              vec3 origin, vec3 forward, vec3 right, int k, col3 *output)
{
   Timer timer("Ray Tracing");

   vec3 up = glm::cross(forward, right);
   vec3 dir = focal_length * forward;
   
   real y = -(yres - 1);
   for (int i = 0; i < yres; ++i)
   {
      real x = -(xres - 1);
      for (int j = 0; j < xres; ++j)
      {
         vec3 d = glm::normalize(dir + x * right + y * up);
         Ray ray { .o = origin, .d = d };
         output[i * xres + j] = rayTrace(ray, rtdata, k);
         x += 2;
      }
      y += 2;
   }
}

col3 rayTrace(Ray ray, RayTracerData *rtdata, int depth)
{
   if (depth == 0)
      return col3(0);

   size_t len = rtdata->tris.size(), ck = -1;
   real ct = std::numeric_limits<real>::infinity();
   for (size_t k = 0; k < len; ++k)
   {
      real t;
      if (rayTriangleIntersection(ray, rtdata->tris[k], &t) && t > EPS && t < ct)
         ct = t, ck = k;
   }

   if (ck == static_cast<size_t>(-1))
      return col3(0);

   vec3 cp = ray.o + ct * ray.d;
   vec3 n = rtdata->normals[ck];
   vec3 r = glm::reflect(ray.d, n);
   const Material &mdata = rtdata->materials[rtdata->mat_indices[ck]];
   col3 diffuse(0), specular(0);

   for (const Light& light : rtdata->lights)
   {
      vec3 l = light.position - cp;
      Ray lr = { .o = cp, .d = l };
      bool block = false;

      for (size_t k = 0; k < len; ++k)
      {
         real t;
         if (rayTriangleIntersection(lr, rtdata->tris[k], &t) && t > EPS && t < 1-EPS)
         {
            block = true;
            break;
         }
      }
      if (block)
         continue;

      real d = glm::length(l);
      l /= d;
      float diff = glm::max(glm::dot(l, n), real(0));
      float d_coeff = 1 / (A*d*d + B*d + C);
      col3 coeff = d_coeff * light.intensity * light.color;
      diffuse += diff * coeff;
      float spec = glm::pow(glm::max(glm::dot(r, l), real(0)), SPECULAR_POW_FACTOR);
      specular += spec * coeff;
   }

   Ray nray = { .o = cp, .d = r };
   return mdata.ka + diffuse * mdata.kd + specular * mdata.ks
      + 0.1f * mdata.ks * rayTrace(nray, rtdata, depth - 1);
}
