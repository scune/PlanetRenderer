#pragma once

#include "Libs.hpp"

#include "Noise.hpp"
#include "ResultCheck.hpp"

/*
 * Info:
 * Halfedges repräsentation eines Objekts benutzen, da sonst keine garantie auf
 * rechtwinklige Dreiecke besteht.
 *
 */

#define BISECTOR_NULLPTR (~uint32_t(0))

struct HalfEdge
{
  uint32_t twin{BISECTOR_NULLPTR};
  uint32_t next{BISECTOR_NULLPTR};
  uint32_t prev{BISECTOR_NULLPTR};
  uint32_t vert;
  uint32_t edge;
  uint32_t face;
};

struct GlobalCBTData
{
  uint32_t depth;
  uint32_t virtualLevels;
  uint32_t firstRealLevel;
  uint32_t minDepth;
  uint32_t baseBisectorIndex;
};

uint32_t MinDepth(uint32_t halfedgeCount) noexcept;

GlobalCBTData InitGlobalCBTData(uint32_t halfedgeCount) noexcept;

struct CBT
{
  std::vector<uint32_t> tree;
  std::vector<uint64_t> bitfield;
};

void InitializeCBT(CBT& cbt, const GlobalCBTData& g,
                   uint32_t halfedgeCount) noexcept;

void SetBitOne(CBT& cbt, uint32_t bitID);
void SetBitZero(CBT& cbt, uint32_t bitID);
uint32_t GetBit(const CBT& cbt, uint32_t bitID);

#define BISECTOR_CMD_KEEP 0
#define BISECTOR_CMD_SPLIT_EDGE_0 1 // Bisector split
#define BISECTOR_CMD_SPLIT_EDGE_1 2 // Right side split
#define BISECTOR_CMD_SPLIT_EDGE_2 4 // Left side split
#define BISECTOR_CMD_SPLIT_ALL                                                 \
  (BISECTOR_CMD_SPLIT_EDGE_0 | BISECTOR_CMD_SPLIT_EDGE_1 |                     \
   BISECTOR_CMD_SPLIT_EDGE_2)
#define BISECTOR_CMD_MERGE_BOUNDARY 8
#define BISECTOR_CMD_MERGE_NON_BOUNDARY 16
#define BISECTOR_CMD_MERGE_SMALLEST_ID 32
#define BISECTOR_CMD_MERGE_ALL                                                 \
  (BISECTOR_CMD_MERGE_BOUNDARY | BISECTOR_CMD_MERGE_NON_BOUNDARY |             \
   BISECTOR_CMD_MERGE_SMALLEST_ID)

inline std::string BisectorCmdToStr(uint32_t cmd)
{
  if (cmd == BISECTOR_CMD_KEEP)
  {
    return "BISECTOR_CMD_KEEP";
  }

  std::string str;
  if (cmd & BISECTOR_CMD_SPLIT_EDGE_0)
  {
    str += "BISECTOR_CMD_SPLIT_EDGE_0";
  }
  if (cmd & BISECTOR_CMD_SPLIT_EDGE_1)
  {
    if (!str.empty())
    {
      str += " | ";
    }
    str += "BISECTOR_CMD_SPLIT_EDGE_1";
  }
  if (cmd & BISECTOR_CMD_SPLIT_EDGE_2)
  {
    if (!str.empty())
    {
      str += " | ";
    }
    str += "BISECTOR_CMD_SPLIT_EDGE_2";
  }
  if (cmd & BISECTOR_CMD_MERGE_BOUNDARY)
  {
    if (!str.empty())
    {
      str += " | ";
    }
    str += "BISECTOR_CMD_MERGE_BOUNDARY";
  }
  if (cmd & BISECTOR_CMD_MERGE_NON_BOUNDARY)
  {
    if (!str.empty())
    {
      str += " | ";
    }
    str += "BISECTOR_CMD_MERGE_NON_BOUNDARY";
  }
  if (cmd & BISECTOR_CMD_MERGE_SMALLEST_ID)
  {
    if (!str.empty())
    {
      str += " | ";
    }
    str += "BISECTOR_CMD_MERGE_SMALLEST_ID";
  }
  return str;
}

struct Bisector
{
  uint32_t next{BISECTOR_NULLPTR};
  uint32_t prev{BISECTOR_NULLPTR};
  uint32_t twin{BISECTOR_NULLPTR};
  uint64_t index; // j, to decode vertex index
  uint32_t command;
  uint32_t unusedMemIDs[4]{BISECTOR_NULLPTR, BISECTOR_NULLPTR, BISECTOR_NULLPTR,
                           BISECTOR_NULLPTR};
};

typedef std::vector<Bisector> BisectorPool;

uint32_t BisectorHalfedgeID(uint32_t index, const GlobalCBTData& g) noexcept;

void QuadHalfedges(std::vector<HalfEdge>& halfedges,
                   std::vector<glm::vec3>& vertices, BisectorPool& bisectors,
                   const GlobalCBTData& g, float scale) noexcept;

void HexagonHalfedges(std::vector<HalfEdge>& halfedges,
                      std::vector<glm::vec3>& vertices, BisectorPool& bisectors,
                      const GlobalCBTData& g, float scale) noexcept;

void ResetCounter(CBT& cbt) noexcept; // Loop 1.

void ResetCommands(BisectorPool& bisectorPool) noexcept; // Loop 3.

void SumReduction(
    CBT& cbt,
    const GlobalCBTData& g) noexcept; // Loop 9. and initialization

typedef std::vector<uint32_t> PointerBuffer;

void CachePointers(PointerBuffer& activeBisectors, const CBT& cbt,
                   const GlobalCBTData& g) noexcept;

void GenerateCommands(const PointerBuffer& activeBisectors,
                      BisectorPool& bisectorPool,
                      const std::vector<HalfEdge>& halfedges,
                      const std::vector<glm::vec3>& vertices, CBT& cbt,
                      const GlobalCBTData& g, const glm::vec3& p0,
                      const glm::vec3& p1) noexcept;

void GenerateCommandRnd(const PointerBuffer& activeBisectors,
                        BisectorPool& bisectorPool, const GlobalCBTData& g);

int32_t GenerateCommandLod(const PointerBuffer& activeBisectors,
                           BisectorPool& bisectorPool,
                           const std::vector<HalfEdge>& halfedges,
                           const std::vector<glm::vec3>& vertices, CBT& cbt,
                           const GlobalCBTData& g, uint32_t maxSubdivision,
                           const glm::vec3& camPos, float scale,
                           const glm::mat4& camMatrix);

void ReserveBlocks(BisectorPool& bisectorPool, const GlobalCBTData& g,
                   CBT& cbt) noexcept;

void FillNewBlocks(BisectorPool& bisectorPool, const GlobalCBTData& g) noexcept;

void UpdateNeighbours(BisectorPool& bisectorPool) noexcept;

void UpdateBitfield(BisectorPool& bisectorPool, CBT& cbt,
                    const GlobalCBTData& g) noexcept;

void GetVertices(CBT& cbt, std::vector<glm::vec4>& vertsOut,
                 std::vector<uint32_t>& indicesOut,
                 const BisectorPool& bisectorPool,
                 const std::vector<HalfEdge>& halfedges,
                 const std::vector<glm::vec3>& vertices,
                 const GlobalCBTData& g) noexcept;

struct CbtLodtestData
{
  std::vector<HalfEdge> halfedges;
  std::vector<glm::vec3> vertices;
  BisectorPool bisectors;
  GlobalCBTData g{};
  CBT cbt;
  bool init{false};
};

inline void PrintParentInfo(uint32_t bisectorID, const BisectorPool& parents,
                            const BisectorPool& oldBisectors)
{
  const Bisector& parent = parents[bisectorID];

  COUT("From bisector " << bisectorID << " with parent cmd: "
                        << BisectorCmdToStr(parent.command));

  if (parent.next != BISECTOR_NULLPTR)
  {
    COUT("Parent next cmd: "
         << BisectorCmdToStr(oldBisectors[parent.next].command));
  }
  if (parent.prev != BISECTOR_NULLPTR)
  {
    COUT("Parent prev cmd: "
         << BisectorCmdToStr(oldBisectors[parent.prev].command));
  }
  if (parent.twin != BISECTOR_NULLPTR)
  {
    COUT("Parent twin cmd: "
         << BisectorCmdToStr(oldBisectors[parent.twin].command));
  }
}

inline void CheckUp(CbtLodtestData& data, const BisectorPool& oldBisectors)
{
  PointerBuffer activeBisectors;
  CachePointers(activeBisectors, data.cbt, data.g);

  BisectorPool parents(oldBisectors.size());
  for (uint32_t i = 0; i < oldBisectors.size(); i++)
  {
    const Bisector& b = oldBisectors[i];
    if (b.index == 0)
      continue;

    for (uint32_t j = 0; j < 4; j++)
    {
      if (b.unusedMemIDs[j] != BISECTOR_NULLPTR)
      {
        if (b.unusedMemIDs[j] < parents.size())
        {
          parents[b.unusedMemIDs[j]] = b;
        }
      }
    }
  }

  ResetCommands(data.bisectors);

  COUT("\n");

  for (uint32_t i = 0; i < activeBisectors.size(); i++)
  {
    const uint32_t bisectorID = activeBisectors[i];
    const Bisector& b = data.bisectors[bisectorID];

    bool warning = false;
    if (PrintWarning(b.index != 0, ".index == 0"))
    {
      warning = true;
    }
    if (PrintWarning(b.next != BISECTOR_NULLPTR, ".next == NULLPTR"))
    {
      warning = true;
    }
    if (PrintWarning(b.prev != BISECTOR_NULLPTR, ".prev == NULLPTR"))
    {
      warning = true;
    }
    if (PrintWarning(b.twin != BISECTOR_NULLPTR, ".twin == NULLPTR"))
    {
      warning = true;
    }

    if (b.next != BISECTOR_NULLPTR)
    {
      data.bisectors[b.next].command = 1;

      if (PrintWarning(GetBit(data.cbt, b.next) == 1,
                       ".next points to uallocated bisector"))
      {
        warning = true;
      }
    }
    if (b.prev != BISECTOR_NULLPTR)
    {
      data.bisectors[b.prev].command = 1;

      if (PrintWarning(GetBit(data.cbt, b.prev) == 1,
                       ".prev points to uallocated bisector"))
      {
        warning = true;
      }
    }
    if (b.twin != BISECTOR_NULLPTR)
    {
      data.bisectors[b.twin].command = 1;

      if (PrintWarning(GetBit(data.cbt, b.twin) == 1,
                       ".twin points to unallocated bisector!"))
      {
        warning = true;
      }
    }

    if (warning)
    {
      PrintParentInfo(bisectorID, parents, oldBisectors);
    }
  }

  for (uint32_t i = 0; i < activeBisectors.size(); i++)
  {
    const uint32_t bisectorID = activeBisectors[i];
    const Bisector& b = data.bisectors[bisectorID];

    // COUT("Bisector " << bisectorID);
    // COUT(".next: " << b.next << " .prev: " << b.prev << " .twin: " <<
    // b.twin);

    if (PrintWarning(
            b.command == 1,
            "Bisector was not tagged by any other neighbour bisector!"))
    {
      PrintParentInfo(bisectorID, parents, oldBisectors);
    }
  }

  COUT("\n");

  ResetCommands(data.bisectors);
}

inline void CbtLodTest(std::vector<glm::vec4>& vertsOut,
                       std::vector<uint32_t>& indicesOut, CbtLodtestData& data,
                       uint32_t maxSubdivision, const glm::vec3& camPos,
                       float scale, const glm::mat4& camMatrix)
{
  if (!data.init)
  {
    const uint32_t halfedgeCount = 4;
    data.g = InitGlobalCBTData(halfedgeCount);
    // HexagonHalfedges(data.halfedges, data.vertices, data.bisectors, data.g,
    //                  scale);
    QuadHalfedges(data.halfedges, data.vertices, data.bisectors, data.g, scale);
    assert(data.halfedges.size() == halfedgeCount);

    InitializeCBT(data.cbt, data.g, data.halfedges.size());
    SumReduction(data.cbt, data.g);

    COUT("Cbt alloc size: " << data.cbt.tree[1]);

    COUT("Min depth: " << data.g.minDepth);
    COUT("Cbt depth: " << data.g.depth);
    COUT("Cbt leaf count: " << data.cbt.bitfield.size() * sizeof(uint64_t));
    COUT("Baseindex: " << data.g.baseBisectorIndex);

    data.init = true;
  }

  ResetCounter(data.cbt);
  ResetCommands(data.bisectors);

  PointerBuffer activeBisectors;
  CachePointers(activeBisectors, data.cbt, data.g);

  COUT("Active bisector count: " << activeBisectors.size());

  int32_t usedMem = GenerateCommandLod(
      activeBisectors, data.bisectors, data.halfedges, data.vertices, data.cbt,
      data.g, maxSubdivision, camPos, scale, camMatrix);
  COUT("Used mem: " << usedMem);

  // GenerateCommandRnd(activeBisectors, data.bisectors, data.g);

  COUT("Reserve blocks");

  ReserveBlocks(data.bisectors, data.g, data.cbt);

  BisectorPool oldBisectors = data.bisectors;

  COUT("Fill new blocks");

  FillNewBlocks(data.bisectors, data.g);

  COUT("Update neighbours");

  UpdateNeighbours(data.bisectors);

  COUT("update bitfield");

  UpdateBitfield(data.bisectors, data.cbt, data.g);

  SumReduction(data.cbt, data.g);

  // CheckUp(data, oldBisectors);

  GetVertices(data.cbt, vertsOut, indicesOut, data.bisectors, data.halfedges,
              data.vertices, data.g);
}
