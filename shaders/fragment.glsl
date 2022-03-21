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
uniform float specular_pow_factor;
uniform float A, B, C; // distance coefficients in Phong model (A*x^2+B*x+C)

void main()
{
   vec3 view_dir = normalize(vp - f_Position);
   vec3 diffuse = vec3(0), specular = vec3(0);
   for (int i = 0; i < light_count; ++i)
   {
      Light light = lights[i];
      vec3 l = light.position - f_Position;
      float d = length(l);
      l /= d;
      float diff = max(dot(l, f_Normal), 0);
      vec3 r = reflect(-l, f_Normal);
      float spec = pow(max(dot(r, view_dir), 0), specular_pow_factor);
      float d_coeff = 1 / (A*d*d + B*d + C);
      vec3 coeff = d_coeff * light.intensity * light.color;
      diffuse += diff * coeff;
      specular += spec * coeff;
   }

   o_Color = vec4(f_Ka + diffuse * f_Kd + specular * f_Ks, 1);
}
