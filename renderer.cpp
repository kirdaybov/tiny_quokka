#include "renderer.h"

void renderer::clear_z()
{
  if (!z_buffer)
    z_buffer = new float[width*height];
  else
    memset(z_buffer, 255, sizeof(float)*width*height);
}

void renderer::triangle(face& f, model& m, pixel* image, float intensity)
{
  vec3 verts[3] = { m.verts[f.v1], m.verts[f.v2], m.verts[f.v3] };
  vec3 uv_verts[3] = { m.uvs[f.t1], m.uvs[f.t2], m.uvs[f.t3] };
  vec3 normals[3] = { m.normals[f.n1], m.normals[f.n2], m.normals[f.n3] };
  if (verts[0].x > verts[1].x) { std::swap(verts[0], verts[1]), std::swap(uv_verts[0], uv_verts[1]); std::swap(normals[0], normals[1]); }
  if (verts[1].x > verts[2].x) { std::swap(verts[1], verts[2]), std::swap(uv_verts[1], uv_verts[2]); std::swap(normals[1], normals[2]); }
  if (verts[0].x > verts[1].x) { std::swap(verts[0], verts[1]), std::swap(uv_verts[0], uv_verts[1]); std::swap(normals[0], normals[1]); }

  for (int j = 0; j < 3; j++)
  {
    verts[j].x = (verts[j].x + 1.f)*width / 2;
    verts[j].y = (verts[j].y + 1.f)*height / 2;
  }

  vec3 dir1 = verts[1] - verts[0];
  vec3 dir2 = verts[2] - verts[0];
  vec3 uv_dir1 = uv_verts[1] - uv_verts[0];
  vec3 uv_dir2 = uv_verts[2] - uv_verts[0];

  float step_x = 0.4f;
  float step_y = 0.4f;
  for (float x = verts[0].x; x <= verts[2].x; x += step_x)
  {
    bool bSecondHalf = x >= verts[1].x;
    if (bSecondHalf) { dir1 = verts[2] - verts[1]; uv_dir1 = uv_verts[2] - uv_verts[1]; }

    float k1 = dir1.x != 0 ? (x - (bSecondHalf ? verts[1].x : verts[0].x)) / dir1.x : 1.f;
    float k2 = dir2.x != 0 ? (x - verts[0].x) / dir2.x : 1.f;
    vec3 p1 = (bSecondHalf ? verts[1] : verts[0]) + dir1*k1;
    vec3 p2 = verts[0] + dir2*k2;

    vec3 uv_p1 = (bSecondHalf ? uv_verts[1] : uv_verts[0]) + uv_dir1*k1;
    vec3 uv_p2 = uv_verts[0] + uv_dir2*k2;

    if (p1.y > p2.y) { std::swap(p1, p2); std::swap(uv_p1, uv_p2); }

    vec3 dir = p2 - p1;
    vec3 uv_dir = uv_p2 - uv_p1;

    for (float y = p1.y; y <= p2.y; y += 1)
    {
      float k = dir.y != 0 ? (y - p1.y) / (p2.y - p1.y) : 1.f;
      vec3 p = p1 + dir*k;
      vec3 uv_p = uv_p1 + uv_dir*k;

      if (p.x > width - 1 ||
        p.y > height - 1 ||
        p.x < 0 || p.y < 0)
        continue;

      int index = int(p.x) + int(p.y)*width;
      if (z_buffer[index] < p.z)
      {
        z_buffer[index] = p.z;
        int dif_index = int(m.d_width*uv_p.x) + m.d_width*int(m.d_height*uv_p.y);
        if (dif_index < 0) dif_index = 0;
        if (dif_index >= m.d_width*m.d_height) dif_index = m.d_width*m.d_height - 1;
        image[index] = m.diffuse ? m.diffuse[dif_index] * intensity : pixel(intensity, intensity, intensity);
      }
    }
  }
}

void renderer::draw_triangular_model(model& a_model, pixel* a_image)
{
  for (int i = 0; i < a_model.faces.size(); i++)
  {
    face f = a_model.faces[i];
    vec3 v[3] = { a_model.verts[f.v1], a_model.verts[f.v2], a_model.verts[f.v3] };

    vec3 e1, e2;
    e1 = v[1] - v[0];
    e2 = v[2] - v[0];

    vec3 normal = vec3::cross(e2, e1);
    normal.normalize();

    vec3 light = { 0, 0, 1 };

    float intensity = vec3::dot(normal, light);

    if (intensity > 0)
      triangle(f, a_model, a_image, intensity);
  }
}

void renderer::render(pixel* image, model &m)
{  
  draw_triangular_model(m, image);
}
