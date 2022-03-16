#version 330 core

struct Light
{
   vec3 position;
   vec3 color;
   float intensity;
};

in vec3 f_Position;
in vec3 f_Normal;
in vec3 f_Ka;
in vec3 f_Kd;
in vec3 f_Ks;

out vec4 o_Color;

uniform vec3 vp;
uniform int light_count;
uniform Light lights[20];
uniform float dist_bound;

vec3 calcColorForLight(Light light, vec3 view_dir)
{
   vec3 l = normalize(light.position - f_Position);
   float diffuse = max(dot(l, f_Normal), 0);
   vec3 r = reflect(-l, f_Normal);
   float specular = pow(max(dot(r, view_dir), 0), 5);
   float d = length(light.position - f_Position) / dist_bound;
   float a = 1, b = 1, c = 0.4;
   float d_coeff = 1 / (a*d*d + b*d + c);
   vec3 coeff = d_coeff * light.intensity * light.color;
   return coeff * (diffuse * f_Kd + specular * f_Ks);
}

void main()
{
   vec3 color = f_Ka;

   vec3 view_dir = normalize(vp - f_Position);
   for (int i = 0; i < light_count; ++i)
      color += calcColorForLight(lights[i], view_dir);
   o_Color = vec4(color, 1);
}
