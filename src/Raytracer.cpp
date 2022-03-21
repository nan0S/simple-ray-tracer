#include "Raytracer.h"

#include "Utils/Timer.h"
#include "Const.h"

#define TEST_CULL
static constexpr float REFLECT_DAMP_FACTOR = 0.1f;

static col3 rayTrace(const Ray &ray, RayTracerData *rtdata, int depth);
static size_t firstIntersection(const Ray &ray, RayTracerData *rtdata, real *ct);

int rayTriangleIntersection(const Ray &ray, const Triangle &tri, real *t)
{
   vec3 pvec = glm::cross(ray.d, tri.bar.v);
   real det = glm::dot(tri.bar.u, pvec);
#ifdef TEST_CULL
   if (det < EPS)
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
         col3 &out_color = output[i * xres + j];
         if (k == 0)
         {
            real ct;
            size_t ck = firstIntersection(ray, rtdata, &ct);
            if (ck == static_cast<size_t>(-1))
               out_color = col3(0);
            else
            {
               const Material &mat = rtdata->materials[rtdata->mat_indices[ck]];
               out_color = mat.ka + mat.kd;
            }
         }
         else
            out_color = rayTrace(ray, rtdata, k);
         x += 2;
      }
      y += 2;
   }
}

col3 rayTrace(const Ray &ray, RayTracerData *rtdata, int depth)
{
   if (depth == 0)
      return col3(0);

   real ct;
   size_t ck = firstIntersection(ray, rtdata, &ct);
   if (ck == static_cast<size_t>(-1))
      return col3(0);

   size_t len = rtdata->tris.size();
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
         if (k != ck && rayTriangleIntersection(lr, rtdata->tris[k], &t) && t > EPS && t < 1-EPS)
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
   col3 color = mdata.ka + diffuse * mdata.kd + specular * mdata.ks;
   float diff = glm::dot(n, r);
   col3 rtcolor = REFLECT_DAMP_FACTOR * (diff * mdata.kd + mdata.ks) * rayTrace(nray, rtdata, depth-1);
   return color + rtcolor;
}

size_t firstIntersection(const Ray &ray, RayTracerData *rtdata, real *ct)
{
   size_t len = rtdata->tris.size(), ck = -1;
   *ct = std::numeric_limits<real>::infinity();
   for (size_t k = 0; k < len; ++k)
   {
      real t;
      if (rayTriangleIntersection(ray, rtdata->tris[k], &t) && t > EPS && t < *ct)
         *ct = t, ck = k;
   }
   return ck;
}
