#include "MeshGen.hpp"

#include <algorithm>
#include <map>

uint32_t GetMiddlePoint(uint32_t p1, uint32_t p2,
                        std::map<std::pair<int, int>, int>& middlePointCache,
                        std::vector<glm::vec3>& vertices)
{
  bool isFirstSmaller = p1 < p2;

  std::pair<int, int> edge =
      isFirstSmaller ? std::make_pair(p1, p2) : std::make_pair(p2, p1);

  if (middlePointCache.count(edge))
  {
    return middlePointCache[edge];
  }

  glm::vec3 middle = (vertices[p1] + vertices[p2]) * 0.5f;

  uint32_t index = vertices.size();
  vertices.push_back(middle);
  middlePointCache[edge] = index;
  return index;
}

void MeshGen::Subdivide(std::vector<glm::vec3>& vertices,
                        std::vector<uint32_t>& indices, uint32_t subdivisions,
                        uint64_t indexOffset) noexcept
{
  std::map<std::pair<int, int>, int> middlePointCache;

  for (uint32_t i = 0; i < subdivisions; i++)
  {
    std::vector<uint32_t> newIndices;
    for (uint64_t i = indexOffset; i < indices.size(); i += 3)
    {
      uint32_t a = GetMiddlePoint(indices[i], indices[i + 1], middlePointCache,
                                  vertices);
      uint32_t b = GetMiddlePoint(indices[i + 1], indices[i + 2],
                                  middlePointCache, vertices);
      uint32_t c = GetMiddlePoint(indices[i + 2], indices[i], middlePointCache,
                                  vertices);

      newIndices.push_back(indices[i]);
      newIndices.push_back(a);
      newIndices.push_back(c);

      newIndices.push_back(indices[i + 1]);
      newIndices.push_back(b);
      newIndices.push_back(a);

      newIndices.push_back(indices[i + 2]);
      newIndices.push_back(c);
      newIndices.push_back(b);

      newIndices.push_back(a);
      newIndices.push_back(b);
      newIndices.push_back(c);
    }
    indices = newIndices;
  }
}

void MeshGen::Icosahedron(std::vector<glm::vec3>& vertices,
                          std::vector<uint32_t>& indices) noexcept
{
  float t = (1.f + std::sqrt(5.f)) / 2.f;

  vertices.reserve(12);

  vertices.emplace_back(glm::vec3(-1, t, 0));
  vertices.emplace_back(glm::vec3(1, t, 0));
  vertices.emplace_back(glm::vec3(-1, -t, 0));
  vertices.emplace_back(glm::vec3(1, -t, 0));

  vertices.emplace_back(glm::vec3(0, -1, t));
  vertices.emplace_back(glm::vec3(0, 1, t));
  vertices.emplace_back(glm::vec3(0, -1, -t));
  vertices.emplace_back(glm::vec3(0, 1, -t));

  vertices.emplace_back(glm::vec3(t, 0, -1));
  vertices.emplace_back(glm::vec3(t, 0, 1));
  vertices.emplace_back(glm::vec3(-t, 0, -1));
  vertices.emplace_back(glm::vec3(-t, 0, 1));

  for (uint64_t i = 0; i < vertices.size(); i++)
  {
    vertices[i] = glm::normalize(vertices[i]);
  }

  indices.reserve(indices.size() + 60);

  // 5 faces around point 0
  indices.emplace_back(0);
  indices.emplace_back(11);
  indices.emplace_back(5);
  indices.emplace_back(0);
  indices.emplace_back(5);
  indices.emplace_back(1);
  indices.emplace_back(0);
  indices.emplace_back(1);
  indices.emplace_back(7);
  indices.emplace_back(0);
  indices.emplace_back(7);
  indices.emplace_back(10);
  indices.emplace_back(0);
  indices.emplace_back(10);
  indices.emplace_back(11);

  // 5 adjacent faces
  indices.emplace_back(1);
  indices.emplace_back(5);
  indices.emplace_back(9);
  indices.emplace_back(5);
  indices.emplace_back(11);
  indices.emplace_back(4);
  indices.emplace_back(11);
  indices.emplace_back(10);
  indices.emplace_back(2);
  indices.emplace_back(10);
  indices.emplace_back(7);
  indices.emplace_back(6);
  indices.emplace_back(7);
  indices.emplace_back(1);
  indices.emplace_back(8);

  // 5 faces around point 3
  indices.emplace_back(3);
  indices.emplace_back(9);
  indices.emplace_back(4);
  indices.emplace_back(3);
  indices.emplace_back(4);
  indices.emplace_back(2);
  indices.emplace_back(3);
  indices.emplace_back(2);
  indices.emplace_back(6);
  indices.emplace_back(3);
  indices.emplace_back(6);
  indices.emplace_back(8);
  indices.emplace_back(3);
  indices.emplace_back(8);
  indices.emplace_back(9);

  // 5 adjacent faces
  indices.emplace_back(4);
  indices.emplace_back(9);
  indices.emplace_back(5);
  indices.emplace_back(2);
  indices.emplace_back(4);
  indices.emplace_back(11);
  indices.emplace_back(6);
  indices.emplace_back(2);
  indices.emplace_back(10);
  indices.emplace_back(8);
  indices.emplace_back(6);
  indices.emplace_back(7);
  indices.emplace_back(9);
  indices.emplace_back(8);
  indices.emplace_back(1);
}

void MeshGen::Icosphere(std::vector<glm::vec3>& vertices,
                        std::vector<uint32_t>& indices,
                        uint32_t subdivisions) noexcept
{
  const uint64_t indexOffset = indices.size();
  Icosahedron(vertices, indices);

  Subdivide(vertices, indices, subdivisions, indexOffset);
}

std::pair<std::array<glm::vec3, 8>, std::array<uint32_t, 36>>
MeshGen::Cube() noexcept
{
  std::array<glm::vec3, 8> vertices{
      glm::vec3(-1.0f, -1.0f, 1.0f),  glm::vec3(1.0f, -1.0f, 1.0f),
      glm::vec3(1.0f, 1.0f, 1.0f),    glm::vec3(-1.0f, 1.0f, 1.0f),
      glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(1.0f, -1.0f, -1.0f),
      glm::vec3(1.0f, 1.0f, -1.0f),   glm::vec3(-1.0f, 1.0f, -1.0f)};

  std::array<uint32_t, 36> indices{0, 1, 2, 2, 3, 0, 1, 5, 6, 6, 2, 1,
                                   4, 7, 6, 6, 5, 4, 0, 3, 7, 7, 4, 0,
                                   3, 2, 6, 6, 7, 3, 4, 1, 0, 5, 1, 4};

  return std::make_pair(vertices, indices);
}

void MeshGen::QuadrilateralizedSphere(std::vector<glm::vec3>& vertices,
                                      std::vector<uint32_t>& indices,
                                      uint32_t subdivisions) noexcept
{
  const uint32_t vertexOffset = vertices.size();
  const uint64_t indexOffset = indices.size();

  const auto cube = Cube();
  vertices.append_range(cube.first);
  indices.append_range(cube.second);

  Subdivide(vertices, indices, subdivisions, indexOffset);

  for (uint32_t i = vertexOffset; i < vertices.size(); i++)
  {
    vertices[i] = glm::normalize(vertices[i]);
  }
}

std::pair<std::array<glm::vec3, 4>, std::array<uint32_t, 6>>
MeshGen::Quad(const float scale) noexcept
{
  std::array<glm::vec3, 4> vertices{
      glm::vec3(-1.f, -1.f, 0.f), glm::vec3(1.f, -1.f, 0.f),
      glm::vec3(1.f, 1.f, 0.f), glm::vec3(-1.f, 1.f, 0.f)};

  for (auto vertex : vertices)
  {
    vertex *= scale;
  }

  std::array<uint32_t, 6> indices{2, 0, 1, 2, 3, 0};

  return std::make_pair(vertices, indices);
}

void MeshGen::QuadHalfedges(std::vector<Cbt::Halfedge>& halfedges,
                            std::vector<glm::vec4>& vertices,
                            float scale) noexcept
{
  halfedges.resize(4);
  vertices.resize(4);

  vertices[0] = glm::vec4(-1.f, -1.f, 0.f, 1.f);
  vertices[1] = glm::vec4(-1.f, 1.f, 0.f, 1.f);
  vertices[2] = glm::vec4(1.f, 1.f, 0.f, 1.f);
  vertices[3] = glm::vec4(1.f, -1.f, 0.f, 1.f);

  for (uint32_t i = 0; i < 4; i++)
  {
    Cbt::Halfedge halfedge{};
    halfedge.twin = BISECTOR_NULLPTR;
    halfedge.next = (i + 3) % 4;
    halfedge.prev = (i + 1) % 4;
    halfedge.vert = i;
    halfedges[i] = halfedge;

    vertices[i] *= scale;
  }
}

void MeshGen::HexagonHalfedges(std::vector<Cbt::Halfedge>& halfedges,
                               std::vector<glm::vec4>& vertices,
                               float scale) noexcept
{
  halfedges.resize(6);
  vertices.resize(6);

  vertices[0] = glm::vec4(1.f, 0.f, 0.f, 1.f);
  vertices[1] = glm::vec4(0.5f, 0.866f, 0.f, 1.f);
  vertices[2] = glm::vec4(-0.5f, 0.866f, 0.f, 1.f);
  vertices[3] = glm::vec4(-1.f, 0.f, 0.f, 1.f);
  vertices[4] = glm::vec4(-0.5f, -0.866f, 0.f, 1.f);
  vertices[5] = glm::vec4(0.5f, -0.866f, 0.f, 1.f);

  for (uint32_t i = 0; i < 6; i++)
  {
    Cbt::Halfedge halfedge{};
    halfedge.twin = BISECTOR_NULLPTR;
    halfedge.next = (i + 5) % 6;
    halfedge.prev = (i + 1) % 6;
    halfedge.vert = i;
    halfedges[i] = halfedge;

    vertices[i] *= scale;
  }
}

void MeshGen::CubeHalfedges(std::vector<Cbt::Halfedge>& halfedges,
                            std::vector<glm::vec4>& vertices,
                            float scale) noexcept
{
  halfedges.resize(4 * 6);
  vertices.resize(8);

  // Top
  vertices[0] = glm::vec4(-1.f, 1.f, 1.f, 1.f);
  vertices[1] = glm::vec4(-1.f, -1.f, 1.f, 1.f);
  vertices[2] = glm::vec4(1.f, -1.f, 1.f, 1.f);
  vertices[3] = glm::vec4(1.f, 1.f, 1.f, 1.f);

  // Bottom
  vertices[4] = glm::vec4(1.f, -1.f, -1.f, 1.f);
  vertices[5] = glm::vec4(-1.f, -1.f, -1.f, 1.f);
  vertices[6] = glm::vec4(-1.f, 1.f, -1.f, 1.f);
  vertices[7] = glm::vec4(1.f, 1.f, -1.f, 1.f);

  for (auto& v : vertices)
  {
    v = glm::vec4(glm::vec3(v) * scale, 1.f);
  }

  for (uint32_t i = 0; i < halfedges.size(); i++)
  {
    Cbt::Halfedge halfedge{};
    halfedge.next = ((i + 1) % 4) + (i / 4) * 4;
    halfedge.prev = ((i + 3) % 4) + (i / 4) * 4;
    halfedges[i] = halfedge;
  }

  // Top
  for (uint32_t i = 0; i < 4; i++)
    halfedges[i].vert = i;

  halfedges[0].twin = 12;
  halfedges[1].twin = 16;
  halfedges[2].twin = 8;
  halfedges[3].twin = 20;

  // Bottom
  for (uint32_t i = 4; i < 8; i++)
    halfedges[i].vert = i;

  halfedges[4].twin = 18;
  halfedges[5].twin = 14;
  halfedges[6].twin = 22;
  halfedges[7].twin = 10;

  // Front
  halfedges[8].twin = 2;
  halfedges[8].vert = 3;

  halfedges[9].twin = 19;
  halfedges[9].vert = 2;

  halfedges[10].twin = 7;
  halfedges[10].vert = 4;

  halfedges[11].twin = 21;
  halfedges[11].vert = 7;

  // Back
  halfedges[12].twin = 0;
  halfedges[12].vert = 1;

  halfedges[13].twin = 23;
  halfedges[13].vert = 0;

  halfedges[14].twin = 5;
  halfedges[14].vert = 6;

  halfedges[15].twin = 17;
  halfedges[15].vert = 5;

  // Left
  halfedges[16].twin = 1;
  halfedges[16].vert = 2;

  halfedges[17].twin = 15;
  halfedges[17].vert = 1;

  halfedges[18].twin = 4;
  halfedges[18].vert = 5;

  halfedges[19].twin = 9;
  halfedges[19].vert = 4;

  // Right
  halfedges[20].twin = 3;
  halfedges[20].vert = 0;

  halfedges[21].twin = 11;
  halfedges[21].vert = 3;

  halfedges[22].twin = 6;
  halfedges[22].vert = 7;

  halfedges[23].twin = 13;
  halfedges[23].vert = 6;

  for (uint32_t i = 0; i < halfedges.size(); i++)
  {
    assert(halfedges[halfedges[i].twin].twin == i);
  }
}

struct VertexPair
{
  VertexPair(uint32_t v0_, uint32_t v1_)
  {
    if (v0_ < v1_)
    {
      v0 = v0_;
      v1 = v1_;
    }
    else
    {
      v0 = v1_;
      v1 = v0_;
    }
  }

  uint32_t v0{0};
  uint32_t v1{0};
};

void MeshGen::TriMeshToHalfedges(const std::vector<uint32_t>& indices,
                                 std::vector<Cbt::Halfedge>& halfedges) noexcept
{
  halfedges.resize(indices.size());

  for (uint32_t i = 0; i < halfedges.size(); i++)
  {
    halfedges[i].twin = BISECTOR_NULLPTR;
    halfedges[i].next = (i + 1) % 3 + (i / 3) * 3;
    halfedges[i].prev = (i + 2) % 3 + (i / 3) * 3;
    halfedges[i].vert = indices[i];
  }

  std::vector<VertexPair> vertexPairs;
  vertexPairs.reserve(halfedges.size() / 2);
  std::vector<std::pair<uint32_t, uint32_t>> twins;
  twins.reserve(halfedges.size() / 2);
  for (uint32_t i = 0; i < halfedges.size(); i++)
  {
    uint32_t v0 = halfedges[i].vert;
    uint32_t v1 = halfedges[halfedges[i].next].vert;
    VertexPair vPair{v0, v1};
    const auto it =
        std::find_if(vertexPairs.cbegin(), vertexPairs.cend(),
                     [vPair](const VertexPair& other)
                     { return vPair.v0 == other.v0 && vPair.v1 == other.v1; });
    if (it != vertexPairs.cend())
    {
      uint32_t vPairIdx = it - vertexPairs.cbegin();
      assert(twins[vPairIdx].second == 0); // No more than 2 twins
      twins[vPairIdx].second = i;

      uint32_t twin = twins[vPairIdx].first;
      halfedges[i].twin = twin;
      halfedges[twin].twin = i;
    }
    else
    {
      vertexPairs.push_back(vPair);
      twins.push_back(std::make_pair(i, 0));
    }
  }
}

void MeshGen::IsosahedronHalfedges(std::vector<Cbt::Halfedge>& halfedges,
                                   std::vector<glm::vec4>& vertices) noexcept
{
  std::vector<glm::vec3> v;
  std::vector<uint32_t> indices;
  Icosphere(v, indices, 3);

  vertices.resize(v.size());
  for (uint32_t i = 0; i < vertices.size(); i++)
  {
    vertices[i] = glm::vec4(glm::normalize(v[i]), 1.f);
  }

  TriMeshToHalfedges(indices, halfedges);
}
