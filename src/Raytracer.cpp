#include "Raytracer.h"

#include "Utils/Timer.h"
#include "Const.h"

void rayTrace(RayTracerData *rtdata, int xres, int yres, float focal_length,
              glm::vec3 origin, glm::vec3 forward, glm::vec3 right,
              float dist_bound, int k, glm::vec3 *output)
{
   Timer timer("Ray Tracing");

   glm::vec3 up = glm::cross(forward, right);
   glm::vec3 dir = focal_length * forward;
   float inv_dist_bound = 1 / dist_bound;
   
   float y = -(yres - 1);
   for (int i = 0; i < yres; ++i)
   {
      float x = -(xres - 1);
      for (int j = 0; j < xres; ++j)
      {
         glm::vec3 d = glm::normalize(dir + x * right + y * up);
         Ray ray { .o = origin, .d = d };
         output[i * xres + j] = rayTrace(ray, rtdata, inv_dist_bound, k);
         x += 2;
      }
      y += 2;
   }
}

glm::vec3 rayTrace(Ray ray, RayTracerData *rtdata, float inv_dist_bound, int depth)
{
   if (depth == 0)
      return glm::vec3(0);

   size_t len = rtdata->tris.size(), ck = -1;
   float ct = std::numeric_limits<float>::infinity();
   for (size_t k = 0; k < len; ++k)
   {
      float t;
      if (rayTriangleIntersection(ray, rtdata->tris[k], &t) && t >= 0.01 && t < ct)
         ct = t, ck = k;
   }

   if (ck == static_cast<size_t>(-1))
      return glm::vec3(0);

   glm::vec3 cp = ray.o + ct * ray.d;
   glm::vec3 n = rtdata->normals[ck];
   glm::vec3 r = glm::reflect(ray.d, n);
   const MeshData &mdata = rtdata->mesh_data[rtdata->mesh_indices[ck]];
   glm::vec3 diffuse(0), specular(0);

   for (const Light& light : rtdata->lights)
   {
      glm::vec3 l = light.position - cp;
      Ray lr = { .o = cp, .d = l };
      bool block = false;

      for (size_t k = 0; k < len; ++k)
      {
         float t;
         if (rayTriangleIntersection(lr, rtdata->tris[k], &t) && t > 0.01 && t < 0.99)
         {
            block = true;
            break;
         }
      }
      if (block)
         continue;

      float d = glm::length(l);
      l /= d;
      d *= inv_dist_bound;
      float diff = glm::max(glm::dot(l, n), 0.f);
      float d_coeff = 1 / (A*d*d + B*d + C);
      glm::vec3 coeff = d_coeff * light.intensity * light.color;
      diffuse += coeff * diff;
      float spec = glm::pow(glm::max(glm::dot(r, l), 0.f), SPECULAR_POW_FACTOR);
      specular += coeff * spec;
   }

   Ray nray = { .o = cp, .d = glm::normalize(r) };
   return mdata.ka + diffuse * mdata.kd + specular * mdata.ks
      + 0.1f * mdata.ks * rayTrace(nray, rtdata, inv_dist_bound, depth - 1);
}
