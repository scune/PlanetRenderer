#include "CbtBisection.hpp"

void QuadHalfedges(std::vector<HalfEdge>& halfedges,
                   std::vector<glm::vec3>& vertices, BisectorPool& bisectors,
                   const GlobalCBTData& g, float scale) noexcept

{
  halfedges.resize(4);
  vertices.resize(4);
  bisectors.resize(1 << g.depth);

  vertices[0] = glm::vec3(-1.f, -1.f, 0.f);
  vertices[1] = glm::vec3(-1.f, 1.f, 0.f);
  vertices[2] = glm::vec3(1.f, 1.f, 0.f);
  vertices[3] = glm::vec3(1.f, -1.f, 0.f);

  for (uint32_t i = 0; i < 4; i++)
  {
    HalfEdge halfedge{};
    halfedge.twin = BISECTOR_NULLPTR;
    halfedge.next = (i + 3) % 4;
    halfedge.prev = (i + 1) % 4;
    halfedge.vert = i;
    halfedge.edge = i;
    halfedge.face = 0;
    halfedges[i] = halfedge;

    Bisector bisector{};
    bisector.next = halfedge.next;
    bisector.prev = halfedge.prev;
    bisector.twin = halfedge.twin;
    bisector.index = g.baseBisectorIndex + i;
    bisectors[i] = bisector;

    vertices[i] *= scale;
  }
}

void HexagonHalfedges(std::vector<HalfEdge>& halfedges,
                      std::vector<glm::vec3>& vertices, BisectorPool& bisectors,
                      const GlobalCBTData& g, float scale) noexcept
{
  halfedges.resize(6);
  vertices.resize(6);
  bisectors.resize(1 << g.depth);

  vertices[0] = glm::vec3(1.f, 0.f, 0.f);
  vertices[1] = glm::vec3(0.5f, 0.866f, 0.f);
  vertices[2] = glm::vec3(-0.5f, 0.866f, 0.f);
  vertices[3] = glm::vec3(-1.f, 0.f, 0.f);
  vertices[4] = glm::vec3(-0.5f, -0.866f, 0.f);
  vertices[5] = glm::vec3(0.5f, -0.866f, 0.f);

  for (uint32_t i = 0; i < 6; i++)
  {
    HalfEdge halfedge{};
    halfedge.twin = BISECTOR_NULLPTR;
    halfedge.next = (i + 5) % 6;
    halfedge.prev = (i + 1) % 6;
    halfedge.vert = i;
    halfedge.edge = i;
    halfedge.face = 0;
    halfedges[i] = halfedge;

    Bisector bisector{};
    bisector.next = halfedge.next;
    bisector.prev = halfedge.prev;
    bisector.twin = halfedge.twin;
    bisector.index = g.baseBisectorIndex + i;
    bisectors[i] = bisector;

    vertices[i] *= scale;
  }
}

uint32_t FastFloorLog2(uint32_t i)
{
  return sizeof(i) * 8 - 1 - __builtin_clz(i); // MSB
}
/*
uint32_t FastCeilLog2(uint32_t i)
{
  return sizeof(i) * 8 - (__builtin_popcount(i) != 0) - __builtin_clz(i);
}*/

uint32_t MinDepth(uint32_t halfedgeCount) noexcept
{
  return FastFloorLog2(halfedgeCount) + 1;
}

GlobalCBTData InitGlobalCBTData(uint32_t halfedgeCount) noexcept
{
  GlobalCBTData g;
  g.depth = 17;
  g.virtualLevels = 6;
  g.firstRealLevel = g.depth - g.virtualLevels - 1;
  g.minDepth = MinDepth(halfedgeCount);
  g.baseBisectorIndex = 1 << g.minDepth;
  return g;
}

void SetBitOne(CBT& cbt, uint32_t bitID)
{
  cbt.bitfield[bitID / 64] |= 1ull << (bitID % 64);
}

void SetBitZero(CBT& cbt, uint32_t bitID)
{
  cbt.bitfield[bitID / 64] &= ~(1ull << (bitID % 64));
}

uint32_t GetBit(const CBT& cbt, uint32_t bitID)
{
  uint32_t localID = bitID % 64;
  return (cbt.bitfield[bitID / 64] >> localID) & 1;
}

uint64_t BitMask(uint32_t count)
{
  assert(count <= 64);
  return (count != 64) ? (1ull << count) - 1 : ~0ull;
}

uint64_t ExtractBits(const CBT& cbt, uint32_t bitID, uint32_t count)
{
  uint32_t localID = bitID % 64;
  assert(localID + count <= 64);

  return (cbt.bitfield[bitID / 64] >> localID) & BitMask(count);
}

void InitializeCBT(CBT& cbt, const GlobalCBTData& g,
                   uint32_t halfedgeCount) noexcept
{
  cbt.tree.resize(1 << (g.firstRealLevel + 1));

  uint32_t bits = std::max<uint32_t>(1, (1 << g.depth) / 64);
  cbt.bitfield.resize(bits);

  for (uint32_t h = 0; h < halfedgeCount; h++)
  {
    SetBitOne(cbt, h);
  }
  COUT("Init cbt");
}

void SumReduction(CBT& cbt, const GlobalCBTData& g) noexcept
{
  // Sum first real level
  uint32_t bitID = 0;
  uint32_t firstRealLevelOffset = 1 << g.firstRealLevel;
  for (uint32_t i = firstRealLevelOffset; i < firstRealLevelOffset * 2; i++)
  {
    cbt.tree[i] = __builtin_popcountll(cbt.bitfield[bitID]) +
                  __builtin_popcountll(cbt.bitfield[bitID + 1]);
    bitID += 2;
  }

  COUT("SumReduction(): summed first real level");

  // Sum rest of tree
  for (int32_t d = g.firstRealLevel - 1; d >= 0; d--)
  {
    for (uint32_t i = 1 << d; i < (1 << (d + 1)); i++)
    {
      cbt.tree[i] = cbt.tree[i << 1] + cbt.tree[(i << 1) + 1];
    }
  }

  COUT("SumReduction(): Tree count: " << cbt.tree[1]);
}

void ResetCounter(CBT& cbt) noexcept { cbt.tree[0] = 0; }

void ResetCommands(BisectorPool& bisectorPool) noexcept
{
  for (auto& bisector : bisectorPool)
  {
    bisector.command = BISECTOR_CMD_KEEP;
  }
}

// clang-format off
const glm::mat3 m0 = glm::transpose(glm::mat3{
    0.f,   0.f, 1.f,
    1.f,   0.f, 0.f,
    0.5f, 0.5f, 0.f});
const glm::mat3 m1 = glm::transpose(glm::mat3{
    0.f,   1.f, 0.f,
    0.f,   0.f, 1.f,
    0.5f, 0.5f, 0.f});
// clang-format on

inline uint32_t BisectorDepth(uint32_t index, const GlobalCBTData& g) noexcept
{
  return FastFloorLog2(index) - g.minDepth;
}

uint32_t BisectorHalfedgeID(uint32_t index, const GlobalCBTData& g) noexcept
{
  return (index >> BisectorDepth(index, g)) - g.baseBisectorIndex;
}

std::array<glm::vec3, 3>
RootBisectorVertices(uint32_t halfedgeID,
                     const std::vector<HalfEdge>& halfedges,
                     const std::vector<glm::vec3>& vertices,
                     const GlobalCBTData& g, const Bisector& bisector) noexcept
{
  halfedgeID -= g.baseBisectorIndex;
  uint32_t next = halfedges[halfedgeID].next;

  const glm::vec3 v0 = vertices[halfedges[halfedgeID].vert];
  const glm::vec3 v1 = vertices[halfedges[next].vert];
  glm::vec3 v2 = v0;

  uint32_t n = 1;
  uint32_t h = next;
  while (h != halfedgeID)
  {
    v2 += vertices[halfedges[h].vert];
    n++;
    h = halfedges[h].next;
  }
  v2 /= n;

  return {v0, v1, v2};
}

glm::mat3 BisectorVertices(const Bisector& bisector,
                           const std::vector<HalfEdge>& halfedges,
                           const std::vector<glm::vec3>& vertices,
                           const GlobalCBTData& g) noexcept
{
  const uint32_t halfedgeID =
      bisector.index >> BisectorDepth(bisector.index, g); // Root bisector index
  glm::mat3 m = glm::identity<glm::mat3>();
  uint32_t h = bisector.index; // Halfedge index

  while (h != halfedgeID) // Compute subd. matrix
  {
    uint8_t b = (h & 1);
    m *= (b ? m1 : m0);
    h >>= 1;
  }

  auto rootVecs =
      RootBisectorVertices(halfedgeID, halfedges, vertices, g, bisector);
  return glm::transpose(
      m * glm::transpose(glm::mat3(rootVecs[0], rootVecs[1], rootVecs[2])));
}

void GetVertices(CBT& cbt, std::vector<glm::vec4>& vertsOut,
                 std::vector<uint32_t>& indicesOut,
                 const BisectorPool& bisectorPool,
                 const std::vector<HalfEdge>& halfedges,
                 const std::vector<glm::vec3>& vertices,
                 const GlobalCBTData& g) noexcept
{
  vertsOut.clear();
  indicesOut.clear();
  uint32_t triID = 0;
  for (uint32_t i = 0; i < bisectorPool.size(); i++)
  {
    if (GetBit(cbt, i) == 0)
      continue;

    const uint32_t bisectorIndex = bisectorPool[i].index;
    // const uint32_t bisectorIndex = i;

    const auto verts =
        BisectorVertices(bisectorPool[i], halfedges, vertices, g);
    vertsOut.push_back(glm::vec4(verts[0], bisectorIndex));
    vertsOut.push_back(glm::vec4(verts[1], bisectorIndex));
    vertsOut.push_back(glm::vec4(verts[2], bisectorIndex));

    indicesOut.append_range(
        std::initializer_list<uint32_t>{triID, triID + 1, triID + 2});
    triID += 3;
  }
}

glm::vec2 BarycentricCoords(const glm::vec3* v, const glm::vec3& p) noexcept
{
  glm::vec3 v0 = v[1] - v[0], v1 = v[2] - v[0], v2 = p - v[0];
  float d00 = glm::dot(v0, v0);
  float d01 = glm::dot(v0, v1);
  float d11 = glm::dot(v1, v1);
  float d20 = glm::dot(v2, v0);
  float d21 = glm::dot(v2, v1);
  float denom = d00 * d11 - d01 * d01;
  glm::vec2 uv;
  uv.x = (d11 * d20 - d01 * d21) / denom;
  uv.y = (d00 * d21 - d01 * d20) / denom;
  return uv;
}

bool OnlySplit0(uint32_t cmd) noexcept
{
  return (
      !(cmd & BISECTOR_CMD_SPLIT_EDGE_1 || cmd & BISECTOR_CMD_SPLIT_EDGE_2) &&
      cmd & BISECTOR_CMD_SPLIT_EDGE_0);
}

inline bool AnySplit(uint32_t cmd) { return cmd & BISECTOR_CMD_SPLIT_ALL; }

inline bool AnyMerge(uint32_t cmd) { return cmd & BISECTOR_CMD_MERGE_ALL; }

int32_t Refine2(BisectorPool& bisectorPool, uint32_t id,
                const GlobalCBTData& g) noexcept
{
  Bisector& b = bisectorPool[id];
  if (AnySplit(b.command) || BisectorDepth(b.index, g) >= g.depth)
    return 0;

  b.command |= BISECTOR_CMD_SPLIT_EDGE_0;
  uint32_t currDepth = BisectorDepth(b.index, g);
  uint32_t twinID = b.twin;

  uint32_t requiredMem = 2;

  while (true)
  {
    if (twinID == BISECTOR_NULLPTR)
      break;

    Bisector& twin = bisectorPool[twinID];
    uint32_t twinDepth = BisectorDepth(twin.index, g);
    const uint32_t oldCmd = twin.command;

    if (currDepth == twinDepth)
    {
      twin.command |= BISECTOR_CMD_SPLIT_EDGE_0;
      COUT(id << ": Refine same depth twin id: " << twinID);

      if (!AnySplit(oldCmd))
      {
        requiredMem += 2;
      }
      break;
    }
    else
    {
      if (twin.next == id)
      {
        COUT(id << ": Refine twin.next == id, twin id: " << twinID);
        twin.command |= BISECTOR_CMD_SPLIT_EDGE_1;
      }
      else
      {
        COUT(id << ": Refine twin.next != id, twin id: " << twinID);
        twin.command |= BISECTOR_CMD_SPLIT_EDGE_2;
      }

      if (AnySplit(oldCmd))
      {
        requiredMem++;
        break;
      }
      else
      {
        requiredMem += 3;

        id = twinID;
        currDepth = twinDepth;
        twinID = twin.twin;
      }
    }
  }

  return requiredMem;
}

void GenerateCommands(const PointerBuffer& activeBisectors,
                      BisectorPool& bisectorPool,
                      const std::vector<HalfEdge>& halfedges,
                      const std::vector<glm::vec3>& vertices, CBT& cbt,
                      const GlobalCBTData& g, const glm::vec3& p0,
                      const glm::vec3& p1) noexcept
{
  for (uint32_t i = 0; i < activeBisectors.size(); i++)
  {
    const uint32_t bisectorID = activeBisectors[i];

    const auto v =
        BisectorVertices(bisectorPool[bisectorID], halfedges, vertices, g);

    const glm::vec2 uv0 = BarycentricCoords(&v[0], p0);
    const glm::vec2 uv1 = BarycentricCoords(&v[0], p1);
    if ((uv0.x >= 0.f && uv0.y >= 0.f && uv0.x + uv0.y <= 1.f) ||
        (uv1.x >= 0.f && uv1.y >= 0.f && uv1.x + uv1.y <= 1.f))
    {
      const uint32_t worstMemAllocSize =
          3 * BisectorDepth(bisectorPool[bisectorID].index, g) + 4; // 3d + 4
      cbt.tree[1] += worstMemAllocSize;
      if (cbt.tree[1] > (1 << g.depth)) // Not enough memory for worst case
      {
        COUT("!!!Not enough memory for worst case!!!");
        cbt.tree[1] -= worstMemAllocSize;
        return;
      }

      Refine2(bisectorPool, bisectorID, g);
    }
  }
}

void TagSmallestId(uint8_t bitValue, Bisector& b1, Bisector& b2)
{
  if (bitValue)
  {
    b2.command |= BISECTOR_CMD_MERGE_SMALLEST_ID;
  }
  else
  {
    b1.command |= BISECTOR_CMD_MERGE_SMALLEST_ID;
  }
}

void TagSmallestId(uint8_t bitValue, Bisector& b1, Bisector& b2, Bisector& b3,
                   Bisector& b4)
{
  if (bitValue)
  {
    if ((b2.index & 3) < (b3.index & 3))
    {
      b2.command |= BISECTOR_CMD_MERGE_SMALLEST_ID;
    }
    else
    {
      b3.command |= BISECTOR_CMD_MERGE_SMALLEST_ID;
    }
  }
  else
  {
    if ((b1.index & 3) < (b4.index & 3))
    {
      b1.command |= BISECTOR_CMD_MERGE_SMALLEST_ID;
    }
    else
    {
      b4.command |= BISECTOR_CMD_MERGE_SMALLEST_ID;
    }
  }
}

int32_t Decimate(BisectorPool& bisectorPool, uint32_t id1,
                 const GlobalCBTData& g) noexcept
{
  Bisector& b1 = bisectorPool[id1];
  if (AnySplit(b1.command) || AnyMerge(b1.command) ||
      BisectorDepth(b1.index, g) == 0)
    return 0;

  const uint8_t bitValue = b1.index & 1;
  uint32_t id2 = (bitValue) ? b1.next : b1.prev;
  uint32_t id3 = (bitValue) ? b1.prev : b1.next;

  if (id2 == BISECTOR_NULLPTR)
    return 0;

  Bisector& b2 = bisectorPool[id2];

  if (b1.index >> 1 == b2.index >> 1) // If same parent
  {
    if (bitValue)
    {
      if (b2.prev != id1)
        return 0;
    }
    else
    {
      if (b2.next != id1)
        return 0;
    }

    if (id3 == BISECTOR_NULLPTR)
    {
      b1.command |= BISECTOR_CMD_MERGE_BOUNDARY;
      b2.command |= BISECTOR_CMD_MERGE_BOUNDARY;
      TagSmallestId(bitValue, b1, b2);
      return 2;
    }
    else
    {
      Bisector& b3 = bisectorPool[id3];
      if (FastFloorLog2(b1.index) == FastFloorLog2(b3.index)) // If same depth
      {
        uint32_t id4 = (bitValue) ? b3.prev : b3.next;
        Bisector& b4 = bisectorPool[id4];
        if (b3.index >> 1 == b4.index >> 1) // If same parent
        {
          if (bitValue)
          {
            if (b4.next != id3)
              return 0;
          }
          else
          {
            if (b4.prev != id3)
              return 0;
          }

          b1.command |= BISECTOR_CMD_MERGE_NON_BOUNDARY;
          b2.command |= BISECTOR_CMD_MERGE_NON_BOUNDARY;
          b3.command |= BISECTOR_CMD_MERGE_NON_BOUNDARY;
          b4.command |= BISECTOR_CMD_MERGE_NON_BOUNDARY;
          TagSmallestId(bitValue, b1, b2, b3, b4);
          return 4;
        }
      }
    }
  }

  return 0;
}

void GenerateCommandRnd(const PointerBuffer& activeBisectors,
                        BisectorPool& bisectorPool, const GlobalCBTData& g)
{
  uint32_t rnd = PcgHash() % (activeBisectors.size() - 1);
  uint32_t bisectorID = activeBisectors[rnd];
  COUT("Refine rnd bisector ID: " << bisectorID);
  Refine2(bisectorPool, bisectorID, g);

  rnd = PcgHash(rnd) % (activeBisectors.size() - 1);
  bisectorID = activeBisectors[rnd];
  COUT("Decimate rnd bisector ID: " << bisectorID);
  Decimate(bisectorPool, bisectorID, g);
}

int32_t GenerateCommandLod(const PointerBuffer& activeBisectors,
                           BisectorPool& bisectorPool,
                           const std::vector<HalfEdge>& halfedges,
                           const std::vector<glm::vec3>& vertices, CBT& cbt,
                           const GlobalCBTData& g, uint32_t maxSubdivision,
                           const glm::vec3& camPos, float scale,
                           const glm::mat4& camMatrix)
{
  maxSubdivision = std::min(maxSubdivision, g.depth);
  int32_t usedMem = cbt.tree[1];
  const int32_t maxMem = 1 << g.depth;
  for (uint32_t i = 0; i < activeBisectors.size(); i++)
  {
    const uint32_t bisectorID = activeBisectors[i];
    const uint32_t depth = BisectorDepth(bisectorPool[bisectorID].index, g);

    const auto v =
        BisectorVertices(bisectorPool[bisectorID], halfedges, vertices, g);

    const glm::vec3 mid = (v[0] + v[1] + v[2]) / 3.f;
    const float distance = glm::distance(camPos, mid) / scale;

    float bisectThreshold = (1.f - float(depth) / maxSubdivision);

    float higherLod = (depth >= 1) ? float(depth - 1) : 0.f;
    float decimateThreshold = 2.f * (1.f - higherLod / maxSubdivision);

    if (distance < bisectThreshold && depth < maxSubdivision)
    {
      if (AnySplit(bisectorPool[bisectorID].command))
        continue;

      int32_t maxRequiredMem = 3 * depth + 4;
      uint32_t twinID = bisectorPool[bisectorID].twin;
      if (twinID == BISECTOR_NULLPTR)
      {
        maxRequiredMem = 2;
      }
      else if (bisectorPool[twinID].twin == bisectorID)
      {
        maxRequiredMem = 4;
      }

      usedMem += maxRequiredMem;
      if (usedMem > maxMem)
      {
        usedMem -= maxRequiredMem;
        COUT("!!!Not enough memory for refinement worst case!!!");
        continue;
      }

      int32_t requiredMem = Refine2(bisectorPool, bisectorID, g);
      usedMem -= std::max(0, maxRequiredMem - requiredMem);
    }
    else if (distance > decimateThreshold && higherLod > 0.f && depth > 0 &&
             bisectorPool[bisectorID].prev != BISECTOR_NULLPTR)
    {
      if (AnySplit(bisectorPool[bisectorID].command) ||
          AnyMerge(bisectorPool[bisectorID].command))
        continue;

      int32_t maxRequiredMem = 4;

      usedMem += maxRequiredMem;
      if (usedMem > maxMem)
      {
        usedMem -= maxRequiredMem;
        COUT("!!!Not enough memory for decimation worst case!!!");
        continue;
      }

      int32_t requiredMem = Decimate(bisectorPool, bisectorID, g);
      usedMem -= std::max(0, maxRequiredMem - requiredMem);
    }
  }
  return usedMem;
}

uint32_t GetCbtBitHelper(uint32_t bitID, const CBT& cbt, const GlobalCBTData& g,
                         uint32_t bitCount)
{
  /*if (bitID < bitCount)
  {
    const uint32_t depth = FastFloorLog2(bitID);
    if (depth < (g.depth - g.virtualLevels)) // Higher than virtual layers
    {
      return cbt.tree[bitID];
    }
    else // Virtual layers
    {
      uint32_t bitExtractCount = g.depth - depth;
      bitID <<= bitExtractCount;
      bitExtractCount = 1 << bitExtractCount;
      bitID -= bitCount;
      return __builtin_popcountg(ExtractBits(cbt, bitID, bitExtractCount));
    }
  }
  else // Bitfield
  {
    return GetBit(cbt, bitID - bitCount);
  }*/
  const uint32_t depth = FastFloorLog2(bitID);
  if (depth < (g.depth - g.virtualLevels))
  {
    return cbt.tree[bitID];
  }
  else // Virtual-layers/Bitfield
  {
    uint32_t bitExtractCount = g.depth - depth;
    bitID <<= bitExtractCount;
    bitExtractCount = 1 << bitExtractCount;
    bitID -= bitCount;
    return __builtin_popcountg(ExtractBits(cbt, bitID, bitExtractCount));
  }
}

uint32_t OneToBitID(uint32_t index, const GlobalCBTData& g,
                    const CBT& cbt) noexcept
{
  uint32_t bitID = 1;
  const uint32_t bitCount = (1 << g.depth);
  while (bitID < bitCount)
  {
    bitID <<= 1;

    uint32_t cbtValue = GetCbtBitHelper(bitID, cbt, g, bitCount);
    if (index >= cbtValue)
    {
      index -= cbtValue;
      bitID++;
    }
  }
  return bitID - bitCount;
}

uint32_t ZeroToBitID(uint32_t index, const GlobalCBTData& g,
                     const CBT& cbt) noexcept
{
  uint32_t bitID = 1;
  const uint32_t bitCount = (1 << g.depth);
  uint32_t c = 1 << (g.depth - 1);
  while (bitID < bitCount)
  {
    bitID <<= 1;

    uint32_t cbtValue = c - GetCbtBitHelper(bitID, cbt, g, bitCount);
    if (index >= cbtValue)
    {
      index -= cbtValue;
      bitID++;
    }
    c >>= 1;
  }
  return bitID - bitCount;
}

void CachePointers(PointerBuffer& activeBisectors, const CBT& cbt,
                   const GlobalCBTData& g) noexcept
{
  activeBisectors.resize(cbt.tree[1]);
  for (uint32_t i = 0; i < cbt.tree[1]; i++)
  {
    activeBisectors[i] = OneToBitID(i, g, cbt);
  }
}

void BisectorResetMergeFlags(Bisector& b)
{
  b.command &= ~BISECTOR_CMD_MERGE_ALL;
}

void ReserveBlocks(BisectorPool& bisectorPool, const GlobalCBTData& g,
                   CBT& cbt) noexcept
{
  for (uint32_t i = 0; i < bisectorPool.size(); i++)
  {
    Bisector& b = bisectorPool[i];
    if (b.command == BISECTOR_CMD_KEEP)
      continue;

    COUT(i << ": " << BisectorCmdToStr(b.command));

    // Reset merge flags if split flags are set
    if (AnySplit(b.command) && AnyMerge(b.command))
    {
      Bisector& b2 = bisectorPool[b.prev];
      BisectorResetMergeFlags(b2);

      if (b.command & BISECTOR_CMD_MERGE_NON_BOUNDARY)
      {
        Bisector& b3 = bisectorPool[b.next];
        BisectorResetMergeFlags(b3);
        Bisector& b4 = bisectorPool[b3.next];
        BisectorResetMergeFlags(b4);
      }

      BisectorResetMergeFlags(b);

      COUT("ANTI MERGE");
    }

    // Reserve blocks
    if (b.command & BISECTOR_CMD_SPLIT_EDGE_1 &&
        b.command & BISECTOR_CMD_SPLIT_EDGE_2)
    {
      for (uint32_t bit = 0; bit < 4; bit++)
      {
        uint32_t bitID = ZeroToBitID(cbt.tree[0] + bit, g, cbt);
        b.unusedMemIDs[bit] = bitID;
      }
      cbt.tree[0] += 4;
    }
    else if (b.command & BISECTOR_CMD_SPLIT_EDGE_1 ||
             b.command & BISECTOR_CMD_SPLIT_EDGE_2)
    {
      for (uint32_t bit = 0; bit < 3; bit++)
      {
        uint32_t bitID = ZeroToBitID(cbt.tree[0] + bit, g, cbt);
        b.unusedMemIDs[bit] = bitID;
      }
      cbt.tree[0] += 3;
    }
    else if (b.command & BISECTOR_CMD_SPLIT_EDGE_0)
    {
      for (uint32_t bit = 0; bit < 2; bit++)
      {
        uint32_t bitID = ZeroToBitID(cbt.tree[0] + bit, g, cbt);
        b.unusedMemIDs[bit] = bitID;
      }
      cbt.tree[0] += 2;
    }
    else if (b.command & BISECTOR_CMD_MERGE_SMALLEST_ID)
    {
      b.unusedMemIDs[0] = ZeroToBitID(cbt.tree[0], g, cbt);
      cbt.tree[0]++;

      // Propagate to merge group
      Bisector& b2 = bisectorPool[b.prev];
      b2.unusedMemIDs[0] = b.unusedMemIDs[0];

      if (b.command & BISECTOR_CMD_MERGE_NON_BOUNDARY)
      {
        b.unusedMemIDs[1] = ZeroToBitID(cbt.tree[0], g, cbt);
        cbt.tree[0]++;

        Bisector& b3 = bisectorPool[b.next];
        b3.unusedMemIDs[0] = b.unusedMemIDs[1];
        Bisector& b4 = bisectorPool[b3.next];
        b4.unusedMemIDs[0] = b.unusedMemIDs[1];
      }
    }
  }
}

void FillNewBlocks(BisectorPool& bisectorPool, const GlobalCBTData& g) noexcept
{
  for (uint32_t i = 0; i < bisectorPool.size(); i++)
  {
    const Bisector& b = bisectorPool[i];
    const uint32_t cmd = b.command;
    const uint32_t* children = b.unusedMemIDs;

    if (cmd & BISECTOR_CMD_SPLIT_EDGE_1 &&
        cmd & BISECTOR_CMD_SPLIT_EDGE_2) // 4 Children
    {
      if (b.next != BISECTOR_NULLPTR)
      {
        PrintWarning(bisectorPool[b.next].command == BISECTOR_CMD_KEEP,
                     "b.next not splitting 1, 2!");
      }
      if (b.prev != BISECTOR_NULLPTR)
      {
        PrintWarning(bisectorPool[b.prev].command == BISECTOR_CMD_KEEP,
                     "b.prev not splitting 1, 2!");
      }
      if (b.twin != BISECTOR_NULLPTR)
      {
        PrintWarning(bisectorPool[b.twin].command == BISECTOR_CMD_KEEP,
                     "b.twin not splitting 1, 2!");
      }

      Bisector lr{}; // Left right
      lr.index = b.index << 2;
      lr.index += 1;
      lr.next = children[2];
      lr.prev = bisectorPool[b.prev].unusedMemIDs[0];
      lr.twin = (b.twin != BISECTOR_NULLPTR)
                    ? bisectorPool[b.twin].unusedMemIDs[1]
                    : BISECTOR_NULLPTR;
      bisectorPool[children[0]] = lr;

      Bisector rl{}; // Right left
      rl.index = b.index << 1;
      rl.index += 1;
      rl.index <<= 1;
      rl.next = bisectorPool[b.next].unusedMemIDs[1];
      rl.prev = children[3];
      rl.twin = (b.twin != BISECTOR_NULLPTR)
                    ? bisectorPool[b.twin].unusedMemIDs[0]
                    : BISECTOR_NULLPTR;
      bisectorPool[children[1]] = rl;

      Bisector ll{}; // Left left
      ll.index = b.index << 2;
      ll.next = bisectorPool[b.prev].unusedMemIDs[1];
      ll.prev = children[0];
      ll.twin = children[3];
      bisectorPool[children[2]] = ll;

      Bisector rr{}; // Right right
      rr.index = b.index << 1;
      rr.index += 1;
      rr.index <<= 1;
      rr.index += 1;
      rr.next = children[1];
      rr.prev = bisectorPool[b.next].unusedMemIDs[0];
      rr.twin = children[2];
      bisectorPool[children[3]] = rr;
    }
    else if (cmd & BISECTOR_CMD_SPLIT_EDGE_1) // Only right split
    {
      Bisector ll{}; // Left
      ll.index = b.index << 1;
      ll.next = (b.twin != BISECTOR_NULLPTR)
                    ? bisectorPool[b.twin].unusedMemIDs[1]
                    : BISECTOR_NULLPTR;
      ll.prev = children[2];
      ll.twin = b.prev;
      bisectorPool[children[0]] = ll;

      Bisector rl{}; // Right left
      rl.index = b.index << 1;
      rl.index += 1;
      rl.index <<= 1;
      rl.next = bisectorPool[b.next].unusedMemIDs[1];
      rl.prev = children[2];
      rl.twin = (b.twin != BISECTOR_NULLPTR)
                    ? bisectorPool[b.twin].unusedMemIDs[0]
                    : BISECTOR_NULLPTR;
      bisectorPool[children[1]] = rl;

      Bisector rr{}; // Right right
      rr.index = b.index << 1;
      rr.index += 1;
      rr.index <<= 1;
      rr.index += 1;
      rr.next = children[1];
      rr.prev = bisectorPool[b.next].unusedMemIDs[0];
      rr.twin = children[0];
      bisectorPool[children[2]] = rr;
    }
    else if (cmd & BISECTOR_CMD_SPLIT_EDGE_2) // Only left split
    {
      Bisector lr{}; // Left right
      lr.index = b.index << 2;
      lr.index += 1;
      lr.next = children[2];
      lr.prev = bisectorPool[b.prev].unusedMemIDs[0];
      lr.twin = (b.twin != BISECTOR_NULLPTR)
                    ? bisectorPool[b.twin].unusedMemIDs[1]
                    : BISECTOR_NULLPTR;
      bisectorPool[children[0]] = lr;

      Bisector r{}; // Right
      r.index = b.index << 1;
      r.index += 1;
      r.next = children[2];
      r.prev = (b.twin != BISECTOR_NULLPTR)
                   ? bisectorPool[b.twin].unusedMemIDs[0]
                   : BISECTOR_NULLPTR;
      r.twin = b.next;
      bisectorPool[children[1]] = r;

      Bisector ll{}; // Left left
      ll.index = b.index << 2;
      ll.next = bisectorPool[b.prev].unusedMemIDs[1];
      ll.prev = children[0];
      ll.twin = children[1];
      bisectorPool[children[2]] = ll;
    }
    else if (cmd & BISECTOR_CMD_SPLIT_EDGE_0)
    {
      if (b.twin != BISECTOR_NULLPTR)
      {
        PrintWarning(AnySplit(bisectorPool[b.twin].command),
                     "b.twin not splitting at split 0!");
      }

      Bisector ll{}; // Left
      ll.index = b.index << 1;
      ll.next = (b.twin != BISECTOR_NULLPTR)
                    ? bisectorPool[b.twin].unusedMemIDs[1]
                    : BISECTOR_NULLPTR;
      ll.prev = children[1];
      ll.twin = b.prev;
      bisectorPool[children[0]] = ll;

      Bisector lr{}; // Right
      lr.index = b.index << 1;
      lr.index += 1;
      lr.next = children[0];
      lr.prev = (b.twin != BISECTOR_NULLPTR)
                    ? bisectorPool[b.twin].unusedMemIDs[0]
                    : BISECTOR_NULLPTR;
      lr.twin = b.next;
      bisectorPool[children[1]] = lr;
    }
    else if (cmd & BISECTOR_CMD_MERGE_SMALLEST_ID)
    {
      if ((b.index & 1) == 1)
      {
        COUT("!!!b.index is on the right: This should not "
             "happen!!!");
      }
      const Bisector& b2 = bisectorPool[b.prev];

      if (cmd & BISECTOR_CMD_MERGE_BOUNDARY)
      {
        Bisector parent{};
        parent.index = b.index >> 1;
        parent.next = b2.twin;
        parent.prev = b.twin;
        parent.twin = BISECTOR_NULLPTR;
        bisectorPool[children[0]] = parent;
      }
      else if (cmd & BISECTOR_CMD_MERGE_NON_BOUNDARY)
      {
        Bisector left{};
        left.index = b.index >> 1;
        left.next = b2.twin;
        left.prev = b.twin;
        left.twin = children[1];
        bisectorPool[children[0]] = left;

        const Bisector& b3 = bisectorPool[b.next];
        const Bisector& b4 = bisectorPool[b3.next];
        if (b4.index > b3.index)
        {
          COUT("!!!otherLeft.index > otherSibling.index: This should not "
               "happen!!!");
          COUT("otherleft index: " << b3.index);
          COUT("otherSibling index: " << b4.index);
        }

        Bisector right{};
        right.index = b4.index >> 1;
        right.next = b3.twin;
        right.prev = b4.twin;
        right.twin = children[0];
        bisectorPool[children[1]] = right;
      }
    }
  }
}

void RefinePointers2(BisectorPool& bisectorPool, const uint32_t id) noexcept
{
  const Bisector& b = bisectorPool[id];
  const uint32_t* children = b.unusedMemIDs;

  if (b.command & BISECTOR_CMD_SPLIT_EDGE_1 &&
      b.command & BISECTOR_CMD_SPLIT_EDGE_2)
  {
    Bisector& next = bisectorPool[b.next];
    uint32_t* nextChildren = next.unusedMemIDs;
    if (next.command & BISECTOR_CMD_SPLIT_EDGE_1 &&
        next.command & BISECTOR_CMD_SPLIT_EDGE_2)
    {
      bisectorPool[nextChildren[0]].twin = children[3];
      bisectorPool[nextChildren[1]].twin = children[1];
    }
    else if (next.command & BISECTOR_CMD_SPLIT_EDGE_1)
    {
      bisectorPool[nextChildren[0]].next = children[3];
      bisectorPool[nextChildren[1]].twin = children[1];
    }
    else if (next.command & BISECTOR_CMD_SPLIT_EDGE_2)
    {
      bisectorPool[nextChildren[0]].twin = children[3];
      bisectorPool[nextChildren[1]].prev = children[1];
    }
    else
    {
      bisectorPool[nextChildren[0]].next = children[3];
      bisectorPool[nextChildren[1]].prev = children[1];
    }

    Bisector& prev = bisectorPool[b.prev];
    uint32_t* prevChildren = prev.unusedMemIDs;
    if (prev.command & BISECTOR_CMD_SPLIT_EDGE_1 &&
        prev.command & BISECTOR_CMD_SPLIT_EDGE_2)
    {
      bisectorPool[prevChildren[0]].twin = children[0];
      bisectorPool[prevChildren[1]].twin = children[2];
    }
    else if (prev.command & BISECTOR_CMD_SPLIT_EDGE_1)
    {
      bisectorPool[prevChildren[0]].next = children[0];
      bisectorPool[prevChildren[1]].twin = children[2];
    }
    else if (prev.command & BISECTOR_CMD_SPLIT_EDGE_2)
    {
      bisectorPool[prevChildren[0]].twin = children[0];
      bisectorPool[prevChildren[1]].prev = children[2];
    }
    else
    {
      bisectorPool[prevChildren[0]].next = children[0];
      bisectorPool[prevChildren[1]].prev = children[2];
    }
  }
  else if (b.command & BISECTOR_CMD_SPLIT_EDGE_1) // Only right
  {
    if (b.prev != BISECTOR_NULLPTR) // Single split side
    {
      Bisector& prev = bisectorPool[b.prev];
      if (AnySplit(prev.command))
      {
        uint32_t* prevChildren = prev.unusedMemIDs;
        bisectorPool[prevChildren[1]].twin = children[0];
      }
      else if (AnyMerge(prev.command))
      {
        uint32_t prevParent = prev.unusedMemIDs[0];
        bisectorPool[prevParent].next = children[0];
      }
      else
      {
        if (prev.next == id)
        {
          prev.next = children[0];
        }
        else
        {
          prev.twin = children[0];
        }
      }
    }

    // Double split side
    Bisector& next = bisectorPool[b.next];
    uint32_t* nextChildren = next.unusedMemIDs;
    if (next.command & BISECTOR_CMD_SPLIT_EDGE_1 &&
        next.command & BISECTOR_CMD_SPLIT_EDGE_2)
    {
      bisectorPool[nextChildren[0]].twin = children[2];
      bisectorPool[nextChildren[1]].twin = children[1];
    }
    else if (next.command & BISECTOR_CMD_SPLIT_EDGE_1)
    {
      bisectorPool[nextChildren[0]].next = children[2];
      bisectorPool[nextChildren[1]].twin = children[1];
    }
    else if (next.command & BISECTOR_CMD_SPLIT_EDGE_2)
    {
      bisectorPool[nextChildren[0]].twin = children[2];
      bisectorPool[nextChildren[1]].prev = children[1];
    }
    else
    {
      bisectorPool[nextChildren[0]].next = children[2];
      bisectorPool[nextChildren[1]].prev = children[1];
    }
  }
  else if (b.command & BISECTOR_CMD_SPLIT_EDGE_2) // Only left
  {
    if (b.next != BISECTOR_NULLPTR) // Single split side
    {
      Bisector& next = bisectorPool[b.next];
      if (AnySplit(next.command))
      {
        uint32_t* nextChildren = next.unusedMemIDs;
        bisectorPool[nextChildren[0]].twin = children[1];
      }
      else if (AnyMerge(next.command))
      {
        uint32_t nextParent = next.unusedMemIDs[0];
        bisectorPool[nextParent].prev = children[1];
      }
      else
      {
        if (next.prev == id)
        {
          next.prev = children[1];
        }
        else
        {
          next.twin = children[1];
        }
      }
    }

    // Double split side
    Bisector& prev = bisectorPool[b.prev];
    uint32_t* prevChildren = prev.unusedMemIDs;
    if (prev.command & BISECTOR_CMD_SPLIT_EDGE_1 &&
        prev.command & BISECTOR_CMD_SPLIT_EDGE_2)
    {
      bisectorPool[prevChildren[0]].twin = children[0];
      bisectorPool[prevChildren[1]].twin = children[2];
    }
    else if (prev.command & BISECTOR_CMD_SPLIT_EDGE_1)
    {
      bisectorPool[prevChildren[0]].next = children[0];
      bisectorPool[prevChildren[1]].twin = children[2];
    }
    else if (prev.command & BISECTOR_CMD_SPLIT_EDGE_2)
    {
      bisectorPool[prevChildren[0]].twin = children[0];
      bisectorPool[prevChildren[1]].prev = children[2];
    }
    else
    {
      bisectorPool[prevChildren[0]].next = children[0];
      bisectorPool[prevChildren[1]].prev = children[2];
    }
  }
  else if (b.command & BISECTOR_CMD_SPLIT_EDGE_0)
  {
    if (b.next != BISECTOR_NULLPTR)
    {
      Bisector& next = bisectorPool[b.next];
      if (AnySplit(next.command))
      {
        uint32_t* nextChildren = next.unusedMemIDs;
        bisectorPool[nextChildren[0]].twin = children[1];
      }
      else if (AnyMerge(next.command))
      {
        uint32_t nextParent = next.unusedMemIDs[0];
        bisectorPool[nextParent].prev = children[1];
      }
      else
      {
        if (next.prev == id)
        {
          next.prev = children[1];
        }
        else
        {
          next.twin = children[1];
        }
      }
    }
    if (b.prev != BISECTOR_NULLPTR)
    {
      Bisector& prev = bisectorPool[b.prev];
      if (AnySplit(prev.command))
      {
        uint32_t* prevChildren = prev.unusedMemIDs;
        bisectorPool[prevChildren[1]].twin = children[0];
      }
      else if (AnyMerge(prev.command))
      {
        uint32_t prevParent = prev.unusedMemIDs[0];
        bisectorPool[prevParent].next = children[0];
      }
      else
      {
        if (prev.next == id)
        {
          prev.next = children[0];
        }
        else
        {
          prev.twin = children[0];
        }
      }
    }
  }
}

void DecimatePointers(BisectorPool& bisectorPool, const uint32_t id0,
                      const uint32_t id1, const uint32_t parent) noexcept
{
  const uint32_t nextID = bisectorPool[id1].twin;

  if (nextID != BISECTOR_NULLPTR)
  {
    Bisector& next = bisectorPool[nextID];

    if (next.prev == id1)
    {
      if (AnySplit(next.command))
      {
        uint32_t nextLeftChild = next.unusedMemIDs[0];
        bisectorPool[nextLeftChild].twin = parent;
      }
      else if (next.command == BISECTOR_CMD_KEEP)
      {
        next.prev = parent;
      }
    }
    else
    {
      if (next.command & BISECTOR_CMD_MERGE_BOUNDARY ||
          next.command & BISECTOR_CMD_MERGE_NON_BOUNDARY)
      {
        uint32_t nextParent = next.unusedMemIDs[0];
        bisectorPool[nextParent].prev = parent;
      }
      else if (next.command == BISECTOR_CMD_KEEP)
      {
        next.twin = parent;
      }
    }
  }

  const uint32_t prevID = bisectorPool[id0].twin;

  if (prevID != BISECTOR_NULLPTR)
  {
    Bisector& prev = bisectorPool[prevID];

    if (prev.next == id0)
    {
      if (AnySplit(prev.command))
      {
        uint32_t prevRightChild = prev.unusedMemIDs[1];
        bisectorPool[prevRightChild].twin = parent;
      }
      else if (prev.command == BISECTOR_CMD_KEEP)
      {
        prev.next = parent;
      }
    }
    else
    {
      if (prev.command & BISECTOR_CMD_MERGE_BOUNDARY ||
          prev.command & BISECTOR_CMD_MERGE_NON_BOUNDARY)
      {
        uint32_t prevParent = prev.unusedMemIDs[0];
        bisectorPool[prevParent].next = parent;
      }
      else if (prev.command == BISECTOR_CMD_KEEP)
      {
        prev.twin = parent;
      }
    }
  }
}

void UpdateNeighbours(BisectorPool& bisectorPool) noexcept
{
  for (uint32_t i = 0; i < bisectorPool.size(); i++)
  {
    Bisector& b = bisectorPool[i];
    const uint32_t cmd = b.command;
    if (cmd == BISECTOR_CMD_KEEP)
      continue;

    if (AnySplit(cmd))
    {
      COUT("Split: " << i);
      RefinePointers2(bisectorPool, i);
      COUT("Split complete");
    }
    else if (cmd & BISECTOR_CMD_MERGE_SMALLEST_ID)
    {
      COUT("Merge: " << i);
      const uint32_t parent0 = b.unusedMemIDs[0];
      const uint32_t siblingID = b.prev;
      DecimatePointers(bisectorPool, i, siblingID, parent0);

      if (cmd & BISECTOR_CMD_MERGE_NON_BOUNDARY)
      {
        const uint32_t id3 = b.next;
        const uint32_t id4 = bisectorPool[id3].next;
        const uint32_t parent1 = b.unusedMemIDs[1];
        DecimatePointers(bisectorPool, id4, id3, parent1);
      }
      COUT("Merge complete");
    }
  }
}

void UpdateBitfield(BisectorPool& bisectorPool, CBT& cbt,
                    const GlobalCBTData& g) noexcept
{
  for (uint32_t i = 0; i < bisectorPool.size(); i++)
  {
    const uint32_t cmd = bisectorPool[i].command;

    if (cmd == BISECTOR_CMD_KEEP)
      continue;

    const uint32_t* children = bisectorPool[i].unusedMemIDs;

    if (cmd & BISECTOR_CMD_SPLIT_EDGE_1 && cmd & BISECTOR_CMD_SPLIT_EDGE_2)
    {
      SetBitOne(cbt, children[0]);
      SetBitOne(cbt, children[1]);
      SetBitOne(cbt, children[2]);
      SetBitOne(cbt, children[3]);
    }
    else if (cmd & BISECTOR_CMD_SPLIT_EDGE_1 || cmd & BISECTOR_CMD_SPLIT_EDGE_2)
    {
      SetBitOne(cbt, children[0]);
      SetBitOne(cbt, children[1]);
      SetBitOne(cbt, children[2]);
    }
    else if (cmd & BISECTOR_CMD_SPLIT_EDGE_0)
    {
      SetBitOne(cbt, children[0]);
      SetBitOne(cbt, children[1]);
    }
    else if (cmd & BISECTOR_CMD_MERGE_SMALLEST_ID)
    {
      if (cmd & BISECTOR_CMD_MERGE_BOUNDARY)
      {
        SetBitOne(cbt, children[0]);
      }
      else if (cmd & BISECTOR_CMD_MERGE_NON_BOUNDARY)
      {
        SetBitOne(cbt, children[0]);
        SetBitOne(cbt, children[1]);
      }
    }

    bisectorPool[i] = Bisector{};
    SetBitZero(cbt, i);
  }
}
