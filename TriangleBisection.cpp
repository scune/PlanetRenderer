#include "TriangleBisection.hpp"

#include "Noise.hpp"

uint64_t HashEdge(glm::vec3 a, glm::vec3 b, const float invNorm)
{
  uint32_t nonUniformDim = 0;
  for (uint32_t i = 0; i < 3; i++)
  {
    if (a[i] != b[i])
    {
      nonUniformDim = i;
    }
  }
  if (a[nonUniformDim] < b[nonUniformDim])
  {
    std::swap(a, b);
  }

  a *= invNorm;
  a = a * 0.5f + 0.5f;
  a *= 1000.f;

  b *= invNorm;
  b = b * 0.5f + 0.5f;
  b *= 1000.f;

  uint32_t hashA = uint32_t(a.x);
  hashA ^= uint32_t(a.y);
  hashA ^= uint32_t(a.z);

  uint32_t hashB = uint32_t(b.x);
  hashB ^= uint32_t(b.y);
  hashB ^= uint32_t(b.z);

  uint64_t combinedHash = (static_cast<uint64_t>(PcgHash(hashA)) << 32) |
                          static_cast<uint64_t>(PcgHash(hashB));
  return combinedHash;
}

std::vector<HalfEdge>
GenHalfedges(const std::vector<glm::vec4>& vertices,
             const std::vector<uint32_t>& indices) noexcept
{
  float maxVertexLength = 1.f;
  for (const auto& vertex : vertices)
  {
    maxVertexLength = std::max(maxVertexLength, glm::length(glm::vec3(vertex)));
  }

  const float invNorm = 1.f / maxVertexLength;

  std::unordered_multimap<uint64_t, uint32_t> map(indices.size());
  for (uint64_t triIdx = 0; triIdx < indices.size(); triIdx += 3)
  {
    const glm::vec3 v[3] = {vertices[indices[triIdx]],
                            vertices[indices[triIdx + 1]],
                            vertices[indices[triIdx + 2]]};

    for (uint8_t i = 0; i < 3; i++)
    {
      uint32_t next = (i + 1) % 3;
      map.insert(std::make_pair<uint64_t, uint32_t>(
          HashEdge(v[i], v[next], invNorm), triIdx + i));
    }
  }

  map.rehash(0);

  std::vector<HalfEdge> halfedges(indices.size());
  for (uint64_t triIdx = 0; triIdx < indices.size(); triIdx += 3)
  {
    const glm::vec3 v[3] = {vertices[indices[triIdx]],
                            vertices[indices[triIdx + 1]],
                            vertices[indices[triIdx + 2]]};

    for (uint32_t i = 0; i < 3; i++)
    {
      const uint64_t halfedgeIdx = triIdx + i;

      const uint64_t key = HashEdge(v[i], v[(i + 1) % 3], invNorm);
      const auto iter = map.equal_range(key);

      halfedges[halfedgeIdx].twin = HALFEDGE_NO_TWIN;
      halfedges[halfedgeIdx].next = triIdx + ((i + 1) % 3);
      halfedges[halfedgeIdx].prev = triIdx + ((i + 2) % 3);
      halfedges[halfedgeIdx].vert = indices[halfedgeIdx];
      halfedges[halfedgeIdx].edge = map.bucket(key);
      halfedges[halfedgeIdx].face = triIdx / 3;

      for (auto it = iter.first; it != iter.second; it++)
      {
        if (it->second != halfedgeIdx &&
            *((glm::vec3*)&vertices[indices[it->second]]) == v[(i + 1) % 3])
        {
          halfedges[halfedgeIdx].twin = it->second;
          // COUT("Twin: " << it->second);
          break;
        }
      }
    }
  }

  return halfedges;
}

/*
 * Treats every input triangle as an counterclockwise isosceles right triangle
 * and orients the hypotenuse to index 0 while keeping it counterclockwise
 */
void OrientIsoscelesRightTriangles(const std::vector<glm::vec4>& vertices,
                                   std::vector<uint32_t>& indices)
{
  for (uint32_t i = 0; i < indices.size(); i += 3)
  {
    const glm::vec4 verts[3] = {vertices[indices[i]], vertices[indices[i + 1]],
                                vertices[indices[i + 2]]};

    const glm::vec3 edges[3] = {(verts[0] - verts[1]), (verts[1] - verts[2]),
                                (verts[2] - verts[0])};

    uint32_t longestEdge = 0;
    for (uint32_t edgeID = 1; edgeID < 3; edgeID++)
    {
      if (glm::length(edges[edgeID]) > glm::length(edges[longestEdge]))
      {
        longestEdge = edgeID;
      }
    }

    // if (longestEdge == 0)
    //   continue;

    if (longestEdge == 1)
    {
      std::swap(indices[i + 0], indices[i + 1]);
      std::swap(indices[i + 1], indices[i + 2]);
    }
    else if (longestEdge == 2) // longestEdge == 2
    {
      std::swap(indices[i + 0], indices[i + 2]);
      std::swap(indices[i + 1], indices[i + 2]);
    }

    //    COUT_VEC3(vertices[indices[i]]);
    //    COUT_VEC3(vertices[indices[i + 1]]);
    //    COUT_VEC3(vertices[indices[i + 2]]);
  }
}

struct Bisector
{
  uint32_t d; // Subdivision level
  uint32_t j; // Index
};

// clang-format off
const glm::mat3 m0 = glm::transpose(glm::mat3{
    1.f,   0.f, 0.f,
    0.f,   0.f, 1.f,
    0.5f, 0.5f, 0.f});
const glm::mat3 m1 = glm::transpose(glm::mat3{
    0.f,   0.f, 1.f,
    0.f,   1.f, 0.f,
    0.5f, 0.5f, 0.f});
// clang-format on

std::array<glm::vec4, 3>
RootBisectorVertices(uint32_t halfedgeID,
                     const std::vector<HalfEdge>& halfedges,
                     const std::vector<glm::vec4>& vertices) noexcept
{
  halfedgeID -= 1;
  uint32_t next = halfedges[halfedgeID].next;
  uint32_t prev = halfedges[halfedgeID].prev;
  return {vertices[halfedges[halfedgeID].vert], vertices[halfedges[next].vert],
          vertices[halfedges[prev].vert]};
}

glm::mat3 BisectorVertices(Bisector bisector,
                           const std::vector<HalfEdge>& halfedges,
                           const std::vector<glm::vec4>& vertices) noexcept
{
  const uint32_t halfedgeID = bisector.j >> bisector.d; // Root bisector index
  glm::mat3 m = glm::identity<glm::mat3>();
  uint32_t h = bisector.j; // Halfedge index

  while (h != halfedgeID) // Compute subd. matrix
  {
    uint8_t b = (h & 1);
    m *= (b == 1) ? m1 : m0;
    h >>= 1;
  }

  auto rootVecs = RootBisectorVertices(halfedgeID, halfedges, vertices);
  return glm::transpose(
      m * glm::transpose(glm::mat3(rootVecs[0], rootVecs[1], rootVecs[2])));
}

std::pair<Bisector, Bisector> Split(Bisector bisector)
{
  Bisector b0 = bisector;
  b0.d += 1;
  b0.j <<= 1;

  Bisector b1 = bisector;
  b1.d += 1;
  b1.j = (b1.j << 1) + 1;

  return {b0, b1};
}

void SplitTest(std::vector<glm::vec4>& vertices, std::vector<uint32_t>& indices,
               uint32_t subdivisions) noexcept
{
  OrientIsoscelesRightTriangles(vertices, indices);

  std::vector<HalfEdge> halfedges = GenHalfedges(vertices, indices);
  std::vector<Bisector> rootBisectors(halfedges.size() / 3);
  for (uint32_t i = 0, j = 1; i < rootBisectors.size(); i++, j += 3)
  {
    rootBisectors[i].j = j; // Only hypotenuse
  }

  std::vector<glm::vec4> newVertices;
  for (uint32_t d = 0; d < subdivisions; d++)
  {
    std::vector<Bisector> bisectors;
    newVertices.clear();
    for (uint32_t i = 0; i < rootBisectors.size(); i++)
    {
      const Bisector& bisector = rootBisectors[i];

      auto splitBisectors = Split(bisector);
      bisectors.append_range(
          std::initializer_list{splitBisectors.first, splitBisectors.second});

      auto bisectorVerts =
          BisectorVertices(splitBisectors.first, halfedges, vertices);
      newVertices.push_back(glm::vec4(bisectorVerts[0], 1.f));
      newVertices.push_back(glm::vec4(bisectorVerts[1], 1.f));
      newVertices.push_back(glm::vec4(bisectorVerts[2], 1.f));

      bisectorVerts =
          BisectorVertices(splitBisectors.second, halfedges, vertices);
      newVertices.push_back(glm::vec4(bisectorVerts[0], 1.f));
      newVertices.push_back(glm::vec4(bisectorVerts[1], 1.f));
      newVertices.push_back(glm::vec4(bisectorVerts[2], 1.f));
    }
    rootBisectors = bisectors;
  }
  vertices = newVertices;

  indices.resize(vertices.size());
  for (uint32_t i = 0; i < indices.size(); i++)
  {
    indices[i] = i;
  }
}

void LEB(std::vector<glm::vec4>& vertices,
         std::vector<uint32_t>& indices) noexcept
{
  std::vector<glm::vec4> newVertices;
  std::vector<uint32_t> newIndices;

  for (uint32_t i = 0; i < indices.size(); i += 3)
  {
    const glm::vec4 verts[3] = {vertices[indices[i]], vertices[indices[i + 1]],
                                vertices[indices[i + 2]]};

    const glm::vec3 edges[3] = {(verts[0] - verts[1]), (verts[1] - verts[2]),
                                (verts[2] - verts[0])};

    uint32_t longestEdge = 0;
    for (uint32_t edgeID = 1; edgeID < 3; edgeID++)
    {
      if (glm::length(edges[edgeID]) > glm::length(edges[longestEdge]))
      {
        longestEdge = edgeID;
      }
    }

    // New vertices
    const uint32_t vertexOffset = newVertices.size();

    for (const auto& vert : verts)
    {
      newVertices.push_back(vert);
    }

    const uint32_t newVertexIndex = newVertices.size();

    glm::vec3 newVertex =
        0.5f * (verts[longestEdge] + verts[(longestEdge + 1) % 3]);
    newVertices.push_back(glm::vec4(newVertex, 1.f));

    // New indices left
    {
      /*
      if (longestEdge == 0)
      {
        newIndices.push_back(vertexOffset + 0);
        newIndices.push_back(newVertexIndex);
        newIndices.push_back(vertexOffset + 2);
      }
      else if (longestEdge == 1)
      {
        newIndices.push_back(vertexOffset + 1);
        newIndices.push_back(newVertexIndex);
        newIndices.push_back(vertexOffset + 0);
      }
      else if (longestEdge == 2)
      {
        newIndices.push_back(vertexOffset + 2);
        newIndices.push_back(newVertexIndex);
        newIndices.push_back(vertexOffset + 1);
      }*/
      newIndices.push_back(vertexOffset + longestEdge);
      newIndices.push_back(newVertexIndex);
      newIndices.push_back(vertexOffset + ((longestEdge + 2) % 3));
    }

    // New indices right
    {
      /*
      if (longestEdge == 0)
      {
        newIndices.push_back(newVertexIndex);
        newIndices.push_back(vertexOffset + 1);
        newIndices.push_back(vertexOffset + 2);
      }
      else if (longestEdge == 1)
      {
        newIndices.push_back(newVertexIndex);
        newIndices.push_back(vertexOffset + 2);
        newIndices.push_back(vertexOffset + 0);
      }
      else if (longestEdge == 2)
      {
        newIndices.push_back(newVertexIndex);
        newIndices.push_back(vertexOffset + 0);
        newIndices.push_back(vertexOffset + 1);
      }*/
      newIndices.push_back(newVertexIndex);
      newIndices.push_back(vertexOffset + ((longestEdge + 1) % 3));
      newIndices.push_back(vertexOffset + ((longestEdge + 2) % 3));
    }
  }

  vertices = newVertices;
  indices = newIndices;
}
