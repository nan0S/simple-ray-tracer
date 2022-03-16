#version 330 core

layout (location = 0) in vec3 v_Position;
layout (location = 1) in vec3 v_Normal;
layout (location = 2) in vec3 v_Ka;
layout (location = 3) in vec3 v_Kd;
layout (location = 4) in vec3 v_Ks;

out vec3 f_Position;
out vec3 f_Normal;
out vec3 f_Ka;
out vec3 f_Kd;
out vec3 f_Ks;

uniform mat4 mvp;

void main()
{
   gl_Position = mvp * vec4(v_Position, 1);

   f_Position = v_Position;
   f_Normal = v_Normal;
   f_Ka = v_Ka;
   f_Kd = v_Kd;
   f_Ks = v_Ks;
}
