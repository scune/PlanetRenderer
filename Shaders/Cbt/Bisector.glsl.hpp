#ifndef BISECTOR_GLSL_HPP
#define BISECTOR_GLSL_HPP

#ifdef __cplusplus
#include "Libs.hpp"
namespace Cbt
{
using namespace glm;
#endif // __cplusplus

struct Bisector
{
  uint next;
  uint prev;
  uint twin;
  uint64_t index;
  uint flags;
  uint unusedMemIDs[4];
};

#define BISECTOR_NULLPTR (~0u)

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // BISECTOR_GLSL_HPP
