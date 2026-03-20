#ifndef GENERIC_GLSL_HPP
#define GENERIC_GLSL_HPP

#include "Bisector.glsl.hpp"
#include "Halfedge.glsl.hpp"

#ifdef __cplusplus
#include "Libs.hpp"
namespace Cbt
{
using namespace glm;
#define CPP_INLINE inline
#else // glsl
#define CPP_INLINE
#endif // __cplusplus

#define CBT_DEPTH 20
#define CBT_VIRTUAL_LEVELS 6
#define CBT_FIRST_REAL_LEVEL (CBT_DEPTH - CBT_VIRTUAL_LEVELS - 1)
#define CBT_TREE_SIZE (1u << (CBT_DEPTH - CBT_VIRTUAL_LEVELS))
#define BISECTOR_MAX_COUNT (1u << CBT_DEPTH)
#define CBT_BITFIELD_SIZE (BISECTOR_MAX_COUNT / 64)

#define BISECTOR_CMD_KEEP 0u
#define BISECTOR_CMD_SPLIT_EDGE_0 1u // Bisector split
#define BISECTOR_CMD_SPLIT_EDGE_1 2u // Right side split
#define BISECTOR_CMD_SPLIT_EDGE_2 4u // Left side split
#define BISECTOR_CMD_SPLIT_ALL                                                 \
  (BISECTOR_CMD_SPLIT_EDGE_0 | BISECTOR_CMD_SPLIT_EDGE_1 |                     \
   BISECTOR_CMD_SPLIT_EDGE_2)
#define BISECTOR_CMD_MERGE_BOUNDARY 8u
#define BISECTOR_CMD_MERGE_NON_BOUNDARY 16u
#define BISECTOR_CMD_MERGE_SMALLEST_ID 32u
#define BISECTOR_CMD_MERGE_ALL                                                 \
  (BISECTOR_CMD_MERGE_BOUNDARY | BISECTOR_CMD_MERGE_NON_BOUNDARY |             \
   BISECTOR_CMD_MERGE_SMALLEST_ID)
#define BISECTOR_CMD_SPLIT 64u
#define BISECTOR_CMD_MERGE 128u

#define BISECTOR_FLAG_TOO_SMALL 1u
#define BISECTOR_FLAG_FRUSTUM_CULLED 2u
#define BISECTOR_FLAG_BACKFACE_CULLED 4u

#define TRIANGLE_SIZE 30.f

CPP_INLINE Bisector InitializeBisector()
{
  Bisector b;
  b.next = BISECTOR_NULLPTR;
  b.prev = BISECTOR_NULLPTR;
  b.twin = BISECTOR_NULLPTR;
  b.index = 0;
  b.flags = 0;
  b.unusedMemIDs[0] = BISECTOR_NULLPTR;
  b.unusedMemIDs[1] = BISECTOR_NULLPTR;
  b.unusedMemIDs[2] = BISECTOR_NULLPTR;
  b.unusedMemIDs[3] = BISECTOR_NULLPTR;
  return b;
}

#ifdef __cplusplus
}
#else
bool AnySplit(uint cmd) { return (cmd & BISECTOR_CMD_SPLIT_ALL) != 0; }

bool AnyMerge(uint cmd) { return (cmd & BISECTOR_CMD_MERGE_ALL) != 0; }

// uint FastFloorLog2(uint64_t i) { return (i != 0) ? uint(findMSB(i)) : 0; }
uint FastFloorLog2(uint64_t i)
{
  int findHigh = findMSB(uint(i >> 32));
  int findLow = findMSB(uint(i));
  int res = (findHigh != -1) ? findHigh + 32 : findLow;
  return uint(max(res, 0));
}

uint BisectorDepth(uint64_t index, uint minDepth)
{
  return FastFloorLog2(index) - minDepth;
}
#endif // __cplusplus

#endif // GENERIC_GLSL_HPP
