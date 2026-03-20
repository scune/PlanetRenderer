#ifndef HALFEDGE_GLSL_HPP
#define HALFEDGE_GLSL_HPP

#ifdef __cplusplus
#include "Libs.hpp"
namespace Cbt
{
using namespace glm;
#endif // __cplusplus

struct Halfedge
{
  uint twin;
  uint next;
  uint prev;
  uint vert;
};

#define BISECTOR_NULLPTR (~0u)

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // HALFEDGE_GLSL_HPP
