#ifndef PLANET_VERTEX_GLSL_HPP
#define PLANET_VERTEX_GLSL_HPP

#ifdef __cplusplus
#include "Libs.hpp"
namespace Cbt
{
using namespace glm;
#endif // __cplusplus

struct PlanetVertex
{
  vec4 pos; // xyz: Position, z: parent pos element
  vec4 normal;
};

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // PLANET_VERTEX_GLSL_HPP
