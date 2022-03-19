#include <fstream>
#include <sstream>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <GL/glew.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/Importer.hpp>
#include <assimp/material.h>

#include "Utils/Log.h"
#include "Utils/Error.h"
#include "Graphics/Shader.h"

#define MAX_LIGHTS 20

using uint = unsigned int;

struct Light
{
   glm::vec3 position;
   glm::vec3 color;
   float intensity;
};

struct Config
{
   std::string comment;
   std::string obj_file_path;
   std::string output_file_path;
   int k;
   int xres, yres;
   glm::vec3 vp;
   glm::vec3 la;
   glm::vec3 up;
   float yview;
   std::vector<Light> lights;
};

struct WindowContext {
   glm::mat4 projection;
   float clip_dist_min;
   float clip_dist_max;
   float fov;
};

struct Triangle
{
   glm::vec3 v[3];
};

struct MeshData
{
   glm::vec3 ka;
   glm::vec3 kd;
   glm::vec3 ks;
};

struct RayTracerData
{
   std::vector<Triangle> tris;
   std::vector<glm::vec3> normals;
   std::vector<uint> mesh_indices;
   std::vector<MeshData> mesh_data;
};

struct RenderData
{
   std::vector<glm::vec3> vertices;
   std::vector<glm::vec3> normals;
   std::vector<glm::vec3> kas;
   std::vector<glm::vec3> kds;
   std::vector<glm::vec3> kss;
   std::vector<uint> indices;
};

static void glfwErrorCallback(int code, const char *desc);
static void windowResizeCallback(GLFWwindow*, int width, int height);
static void keyInputCallback(GLFWwindow* window, int key, int, int action, int);

template<class T> std::ostream& operator<<(std::ostream &out, const glm::vec<3, T>& v);
template<class T> std::istream& operator>>(std::istream &in, glm::vec<3, T> &v);

static const char *USAGE_STR =
"Usage: ./raytracer CONFIG_FILE\n\n"
"Confiration file template:\n\n"
"comment\n"
"path/to/file.obj\n"
"path/to/file.png\n"
"k_parameter\n"
"x_resolution y_resolution\n"
"view_point_x view_point_y view_point_z\n"
"look_at_x look_at_y look_at_z\n"
"[up_x up_y up_z] (default=[0,1,0])\n"
"[yview] (default=1)\n"
"[L light_pos_x light_pos_y light_pos_y light_col_x light_col_y intensity]...";

static const char *INSTRUCTION_STR =
"Use WASD to move, MOUSE to look around.\n"
"Press U to update the current configuration.\n"
"Press LEFT MOUSE BUTTON to print the current position (useful for changing script manually).\n"
"Press ESCAPE/Q to quit.";

static constexpr int WINDOW_WIDTH = 1280;
static constexpr int WINDOW_HEIGHT = 720;
static constexpr float CLIP_DIST_MIN_FACTOR = 0.0001f;
static constexpr float CLIP_DIST_MAX_FACTOR = 20;
static constexpr float MOVEMENT_SPEED_FACTOR = 0.45f;
static constexpr float LOOK_SENSITIVITY = 0.15f;

int main(int argc, char *argv[])
{
   if (argc != 2)
      ERROR(USAGE_STR);
   const char *config_file_path = argv[1];

   /* Parse configuration. */
   Config config;
   {
      std::ifstream config_file(config_file_path);
      if (!config_file.is_open())
         ERROR("Failed to open configuration file.");

      config.up = glm::vec3(0, 1, 0);
      config.yview = 1;

      std::getline(config_file, config.comment); // ignore comment line
      std::getline(config_file, config.obj_file_path);
      std::getline(config_file, config.output_file_path);
      std::string line;
      {
         std::getline(config_file, line);
         config.k = std::stoi(line);
      }
      {
         std::getline(config_file, line);
         std::stringstream ss(line);
         ss >> config.xres >> config.yres;
      }
      {
         std::getline(config_file, line);
         std::stringstream ss(line);
         ss >> config.vp;
      }
      {
         std::getline(config_file, line);
         std::stringstream ss(line);
         ss >> config.la;
      }
      if (std::getline(config_file, line))
      {
         {
            std::stringstream ss(line);
            ss >> config.up;
         }
         if (std::getline(config_file, line))
         {
            {
               std::stringstream ss(line);
               ss >> config.yview;
            }
            while (std::getline(config_file, line)) {
               std::stringstream ss(line);
               char c;
               if (!(ss >> c) || c != 'L')
                  break;
               Light &light = config.lights.emplace_back();
               ss >> light.position >> light.color >> light.intensity;
               light.color /= 255;
               light.intensity *= 0.01f;
            }
         }
      }
   }

   if (config.lights.size() > MAX_LIGHTS)
      ERROR("Too many lights in the scene.");

   /* Initialize OpenGL. */
   glfwSetErrorCallback(glfwErrorCallback);
   if (!glfwInit())
      ERROR("Failed to initialize GLFW.");

   glfwWindowHint(GLFW_SAMPLES, 4);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
   glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
   glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
   glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
   GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT,
                                         "SimpleRayTracer", NULL, NULL);
   if (!window)
      ERROR("Failed to create GLFW window.");
   
   WindowContext window_context;
   glfwMakeContextCurrent(window);
   glfwSetWindowUserPointer(window, (void*)&window_context);
   glfwSetFramebufferSizeCallback(window, windowResizeCallback);
   glfwSetKeyCallback(window, keyInputCallback);
   // glfwSwapInterval(0); // Unlimited FPS.
   glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

   if (glewInit() != GLEW_OK)
      ERROR("Failed to initialize GLEW.");

   GL_CALL(glClearColor(0, 0, 0, 1));
   GL_CALL(glClear(GL_COLOR_BUFFER_BIT));
   GL_CALL(glEnable(GL_DEPTH_TEST));
   GL_CALL(glEnable(GL_CULL_FACE));

   /* Prepare program variables, load assets ... */
   RayTracerData rtdata;
   int indices_count;
   const float inf = 1e18f;
   glm::vec3 min_point(inf, inf, inf);
   glm::vec3 max_point(-inf, -inf, -inf);
   {
      RenderData rdata;
      {
         Assimp::Importer importer;
         const aiScene *scene = importer.ReadFile(config.obj_file_path.c_str(),
                                                  aiProcess_Triangulate | aiProcess_GenNormals |
                                                  aiProcess_FlipUVs | aiProcess_JoinIdenticalVertices);
         if (!scene)
            ERROR(importer.GetErrorString());

         uint n_tris = 0, n_vertices = 0;
         for (uint i = 0; i < scene->mNumMeshes; ++i)
         {
            const aiMesh *mesh = scene->mMeshes[i];
            n_tris += mesh->mNumFaces;
            n_vertices += mesh->mNumVertices;
         }

         rtdata.tris.reserve(n_tris);
         rtdata.normals.reserve(n_tris);
         rtdata.mesh_indices.reserve(n_tris);
         rtdata.mesh_data.reserve(scene->mNumMeshes);

         rdata.vertices.reserve(n_vertices);
         rdata.normals.reserve(n_vertices);
         rdata.kas.reserve(n_vertices);
         rdata.kds.reserve(n_vertices);
         rdata.kss.reserve(n_vertices);
         rdata.indices.reserve(n_tris * 3);

         uint index_offset = 0;
         for (uint i = 0; i < scene->mNumMeshes; ++i)
         {
            const aiMesh *mesh = scene->mMeshes[i];
            const aiVector3D *verts = mesh->mVertices;
            const aiVector3D *normals = mesh->mNormals;
            const aiMaterial *mat = scene->mMaterials[mesh->mMaterialIndex];

            aiColor3D ka, kd, ks;
            mat->Get(AI_MATKEY_COLOR_AMBIENT, ka);
            mat->Get(AI_MATKEY_COLOR_DIFFUSE, kd);
            mat->Get(AI_MATKEY_COLOR_SPECULAR, ks);

            MeshData mesh_data {
               glm::vec3(ka.r, ka.g, ka.b),
               glm::vec3(kd.r, kd.g, kd.b),
               glm::vec3(ks.r, ks.g, ks.b)
            };
            rtdata.mesh_data.push_back(mesh_data);

            for (uint j = 0; j < mesh->mNumFaces; ++j)
            {
               aiFace face = mesh->mFaces[j];
               Triangle &tri = rtdata.tris.emplace_back();
               assert(face.mNumIndices == 3);

               for (uint k = 0; k < face.mNumIndices; ++k)
               {
                  uint idx = face.mIndices[k];
                  tri.v[k] = glm::vec3(verts[idx].x, verts[idx].y, verts[idx].z);
                  rdata.indices.push_back(index_offset + idx);
               }
               {
                  uint idx = face.mIndices[0];
                  rtdata.normals.push_back(glm::vec3(normals[idx].x,
                                                     normals[idx].y,
                                                     normals[idx].z));
               }
               rtdata.mesh_indices.push_back(i);
            }

            for (uint j = 0; j < mesh->mNumVertices; ++j)
            {
               aiVector3D vertex = verts[j];
               aiVector3D normal = normals[j];

               rdata.vertices.push_back(glm::vec3(vertex.x, vertex.y, vertex.z));
               rdata.normals.push_back(glm::vec3(normal.x, normal.y, normal.z));
               rdata.kas.push_back(mesh_data.ka);
               rdata.kds.push_back(mesh_data.kd);
               rdata.kss.push_back(mesh_data.ks);

               max_point.x = std::max(max_point.x, vertex.x);
               max_point.y = std::max(max_point.y, vertex.y);
               max_point.z = std::max(max_point.z, vertex.z);
               min_point.x = std::min(min_point.x, vertex.x);
               min_point.y = std::min(min_point.y, vertex.y);
               min_point.z = std::min(min_point.z, vertex.z);
            }

            index_offset += mesh->mNumVertices;
         }
      }

      /* Setup buffers and variables. */
      {
         GLuint vao;
         GL_CALL(glGenVertexArrays(1, &vao));
         GL_CALL(glBindVertexArray(vao));

         GLuint vvbo;
         GL_CALL(glGenBuffers(1, &vvbo));
         GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, vvbo));
         GL_CALL(glBufferData(GL_ARRAY_BUFFER,
                              rdata.vertices.size() * sizeof(glm::vec3),
                              rdata.vertices.data(),
                              GL_STATIC_DRAW));
         GL_CALL(glEnableVertexAttribArray(0));
         GL_CALL(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0));

         GLuint nvbo;
         GL_CALL(glGenBuffers(1, &nvbo));
         GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, nvbo));
         GL_CALL(glBufferData(GL_ARRAY_BUFFER,
                              rdata.normals.size() * sizeof(glm::vec3),
                              rdata.normals.data(),
                              GL_STATIC_DRAW));
         GL_CALL(glEnableVertexAttribArray(1));
         GL_CALL(glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0));

         GLuint kavbo;
         GL_CALL(glGenBuffers(1, &kavbo));
         GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, kavbo));
         GL_CALL(glBufferData(GL_ARRAY_BUFFER,
                              rdata.kas.size() * sizeof(glm::vec3),
                              rdata.kas.data(),
                              GL_STATIC_DRAW));
         GL_CALL(glEnableVertexAttribArray(2));
         GL_CALL(glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0));

         GLuint kdvbo;
         GL_CALL(glGenBuffers(1, &kdvbo));
         GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, kdvbo));
         GL_CALL(glBufferData(GL_ARRAY_BUFFER,
                              rdata.kds.size() * sizeof(glm::vec3),
                              rdata.kds.data(),
                              GL_STATIC_DRAW));
         GL_CALL(glEnableVertexAttribArray(3));
         GL_CALL(glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0));

         GLuint ksvbo;
         GL_CALL(glGenBuffers(1, &ksvbo));
         GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, ksvbo));
         GL_CALL(glBufferData(GL_ARRAY_BUFFER,
                              rdata.kss.size() * sizeof(glm::vec3),
                              rdata.kss.data(),
                              GL_STATIC_DRAW));
         GL_CALL(glEnableVertexAttribArray(4));
         GL_CALL(glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, 0));

         GLuint ebo;
         GL_CALL(glGenBuffers(1, &ebo));
         GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo));
         GL_CALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                              rdata.indices.size() * sizeof(uint),
                              rdata.indices.data(),
                              GL_STATIC_DRAW));
      }

      indices_count = static_cast<int>(rdata.indices.size());
   }

   GLuint shader = Graphics::loadGraphicsShader("shaders/vertex.glsl", "shaders/fragment.glsl");
   GL_CALL(glUseProgram(shader));
   GL_CALL(GLint mvp_loc = glGetUniformLocation(shader, "mvp"));
   GL_CALL(GLint vp_loc = glGetUniformLocation(shader, "vp"));
   {
      GL_CALL(GLint light_count_loc = glGetUniformLocation(shader, "light_count"));
      GL_CALL(glUniform1i(light_count_loc, static_cast<int>(config.lights.size())));
   }
   for (size_t i = 0; i < config.lights.size(); ++i)
   {
      std::string location_str_base = "lights[" + std::to_string(i) + "].";
      Light &light = config.lights[i];
      {
         std::string position_str = location_str_base + "position";
         GL_CALL(GLint position_loc = glGetUniformLocation(shader, position_str.c_str()));
         GL_CALL(glUniform3f(position_loc, light.position.x, light.position.y, light.position.z));
      }
      {
         std::string color_str = location_str_base + "color";
         GL_CALL(GLint color_loc = glGetUniformLocation(shader, color_str.c_str()));
         GL_CALL(glUniform3f(color_loc, light.color.x, light.color.y, light.color.z));
      }
      {
         std::string intensity_str = location_str_base + "intensity";
         GL_CALL(GLint intensity_loc = glGetUniformLocation(shader, intensity_str.c_str()));
         GL_CALL(glUniform1f(intensity_loc, light.intensity));
      }
   }
   float dist_bound = glm::length(max_point - min_point);
   {
      GL_CALL(GLint dist_bound_loc = glGetUniformLocation(shader, "dist_bound"));
      GL_CALL(glUniform1f(dist_bound_loc, dist_bound));
   } 

   glm::vec3 position = config.vp;
   glm::vec3 forward = glm::normalize(config.la - position);
   glm::vec3 up = glm::normalize(config.up);
   glm::vec3 right = glm::cross(up, forward);

   float vert_rotation = 0;
   double last_xpos, last_ypos;
   glfwGetCursorPos(window, &last_xpos, &last_ypos);

   float movement_speed = dist_bound * MOVEMENT_SPEED_FACTOR;
   window_context.clip_dist_min = dist_bound * CLIP_DIST_MIN_FACTOR;
   window_context.clip_dist_max = dist_bound * CLIP_DIST_MAX_FACTOR;
   window_context.fov = 2 * std::atan(0.5f / config.yview);
   windowResizeCallback(window, WINDOW_WIDTH, WINDOW_HEIGHT);

   int u_last_state = GLFW_RELEASE;
   int left_last_state = GLFW_RELEASE;

   print(INSTRUCTION_STR);

   /* Main loop. */
   while (!glfwWindowShouldClose(window))
   {
      /* Calculate delta time. */
      float delta_time;
      {
         static float last_time = static_cast<float>(glfwGetTime());
         float now = static_cast<float>(glfwGetTime());
         delta_time = now - last_time;
         last_time = now;
      }
      /* Calculate mouse movement. */
      float delta_hor, delta_vert;
      {
         static constexpr float pi_2 = static_cast<float>(M_PI_2);
         double xpos, ypos;
         glfwGetCursorPos(window, &xpos, &ypos);
         delta_hor = delta_time * LOOK_SENSITIVITY * static_cast<float>(xpos - last_xpos);
         delta_vert = delta_time * LOOK_SENSITIVITY * static_cast<float>(ypos - last_ypos);
         delta_vert = glm::clamp<float>(vert_rotation + delta_vert, -pi_2, pi_2) - vert_rotation;
         vert_rotation += delta_vert;
         last_xpos = xpos;
         last_ypos = ypos;
      }
      /* Calculate keyboard input. */
      {
         glm::vec3 delta_pos(0);
         if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            delta_pos += forward;
         if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            delta_pos -= forward;
         if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            delta_pos += right;
         if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            delta_pos -= right;
         if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
            delta_pos += up;
         if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
            delta_pos -= up;
         position += (delta_time * movement_speed) * delta_pos;
         GL_CALL(glUniform3f(vp_loc, position.x, position.y, position.z));
      }
      /* Update camera view. */
      {
         glm::mat4 rot = glm::rotate(-delta_hor, up);
         forward = rot * glm::rotate(delta_vert, right) * glm::vec4(forward, 0);
         right = rot * glm::vec4(right, 0);
         glm::vec3 visual_up = glm::cross(forward, right);
         glm::mat4 view = glm::lookAt(position, position + forward, visual_up);
         glm::mat4 mvp = window_context.projection * view;
         GL_CALL(glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, &mvp[0][0]));
      }
      /* Respond to other commands. */
      {
         /* Update configuration. */
         {
            int u_state = glfwGetKey(window, GLFW_KEY_U);
            if (u_last_state == GLFW_RELEASE && u_state == GLFW_PRESS)
            {
               std::ofstream out(config_file_path);
               glm::vec3 la = position + forward;
               out << config.comment << '\n';
               out << config.obj_file_path << '\n';
               out << config.output_file_path << '\n';
               out << config.k << '\n';
               out << config.xres << ' ' << config.yres << '\n';
               out << position.x << ' ' << position.y << ' ' << position.z << '\n';
               out << la.x << ' ' << la.y << ' ' << la.z << '\n';
               out << config.up.x << ' ' << config.up.y << ' ' << config.up.z << '\n';
               out << config.yview << '\n';
               for (Light &l : config.lights)
               {
                  glm::ivec3 color(l.color.x * 255, l.color.y * 255, l.color.z * 255);
                  out << "L " << l.position.x << ' ' << l.position.y << ' ' << l.position.z << ' ';
                  out << color.x << ' ' << color.y << ' ' << color.z << ' ';
                  out << l.intensity * 100 << '\n';
               }
               print("Configuration updated.");
            }
            u_last_state = u_state;
         }
         /* Print info. */
         {
            int left_state = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
            if (left_last_state == GLFW_RELEASE && left_state == GLFW_PRESS)
               print(position);
            left_last_state = left_state;
         }
      }

      GL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
      GL_CALL(glDrawElements(GL_TRIANGLES, indices_count, GL_UNSIGNED_INT, 0));
      glfwPollEvents();
      glfwSwapBuffers(window);
   }

   return 0;
}

void glfwErrorCallback(int code, const char *desc)
{
   ERROR("[GLFW Error] '", desc, "' (", code, ")");
}

void windowResizeCallback(GLFWwindow* window, int width, int height)
{
   WindowContext* ctx = (WindowContext*)glfwGetWindowUserPointer(window);
   float aspect = static_cast<float>(width) / static_cast<float>(height);
   ctx->projection = glm::perspective(ctx->fov, aspect, ctx->clip_dist_min, ctx->clip_dist_max);
   GL_CALL(glViewport(0, 0, width, height));
}

void keyInputCallback(GLFWwindow* window, int key, int, int action, int)
{
   switch (key)
   {
      case GLFW_KEY_ESCAPE:
      case GLFW_KEY_Q:
         if (action == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GL_TRUE);
         break;
   }
}

template<class T>
std::ostream& operator<<(std::ostream &out, const glm::vec<3, T>& v)
{
   return out << v.x << ' ' << v.y << ' ' << v.z;
}

template<class T>
std::istream& operator>>(std::istream &in, glm::vec<3, T> &v)
{
   return in >> v.x >> v.y >> v.z;
}
