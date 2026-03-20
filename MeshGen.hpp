#pragma once

#include "Libs.hpp"

#include "Shaders/Cbt/Halfedge.glsl.hpp"

namespace MeshGen
{
void Subdivide(std::vector<glm::vec3>& vertices, std::vector<uint32_t>& indices,
               uint32_t subdivisions, uint64_t indexOffset) noexcept;

void Icosahedron(std::vector<glm::vec3>& vertices,
                 std::vector<uint32_t>& indices) noexcept;

void Icosphere(std::vector<glm::vec3>& vertices, std::vector<uint32_t>& indices,
               uint32_t subdivisions) noexcept;

std::pair<std::array<glm::vec3, 8>, std::array<uint32_t, 36>> Cube() noexcept;

void QuadrilateralizedSphere(std::vector<glm::vec3>& vertices,
                             std::vector<uint32_t>& indices,
                             uint32_t subdivisions) noexcept;

std::pair<std::array<glm::vec3, 4>, std::array<uint32_t, 6>>
Quad(const float scale = 1.f) noexcept;

void QuadHalfedges(std::vector<Cbt::Halfedge>& halfedges,
                   std::vector<glm::vec4>& vertices, float scale) noexcept;

void HexagonHalfedges(std::vector<Cbt::Halfedge>& halfedges,
                      std::vector<glm::vec4>& vertices, float scale) noexcept;

void CubeHalfedges(std::vector<Cbt::Halfedge>& halfedges,
                   std::vector<glm::vec4>& vertices, float scale) noexcept;

void TriMeshToHalfedges(const std::vector<uint32_t>& indices,
                        std::vector<Cbt::Halfedge>& halfedges) noexcept;

void IsosahedronHalfedges(std::vector<Cbt::Halfedge>& halfedges,
                          std::vector<glm::vec4>& vertices) noexcept;
} // namespace MeshGen
