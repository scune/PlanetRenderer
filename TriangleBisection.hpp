#pragma once

#include "Libs.hpp"

#define HALFEDGE_NO_TWIN (~uint32_t(0))

struct HalfEdge
{
  uint32_t twin{HALFEDGE_NO_TWIN};
  uint32_t next;
  uint32_t prev;
  uint32_t vert;
  uint32_t edge;
  uint32_t face;
};

std::vector<HalfEdge>
GenHalfedges(const std::vector<glm::vec4>& vertices,
             const std::vector<uint32_t>& indices) noexcept;

void SplitTest(std::vector<glm::vec4>& vertices, std::vector<uint32_t>& indices,
               uint32_t subdivisions) noexcept;

void LEB(std::vector<glm::vec4>& vertices,
         std::vector<uint32_t>& indices) noexcept;
