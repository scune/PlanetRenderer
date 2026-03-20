#ifndef GET_CBT_BIT_HELPER_GLSL
#define GET_CBT_BIT_HELPER_GLSL

uint64_t BitMask(uint count)
{
  return (count < 64) ? (1ul << count) - 1ul : ~0ul;
}

uint64_t ExtractBits(uint bitID, uint count)
{
  uint localID = bitID % 64;
  return (cbt.bitfield[bitID / 64] >> localID) & BitMask(count);
}

uint GetCbtBitHelper(uint bitID)
{
  const uint depth = FastFloorLog2(bitID);
  if (depth < CBT_DEPTH - CBT_VIRTUAL_LEVELS)
  {
    return cbt.tree[bitID];
  }
  else // Virtual-layers/Bitfield
  {
    uint bitExtractCount = CBT_DEPTH - depth;
    bitID <<= bitExtractCount;
    bitExtractCount = 1 << bitExtractCount;
    bitID -= BISECTOR_MAX_COUNT;
    return uint(bitCount(ExtractBits(bitID, bitExtractCount)));
  }
}

#endif // GET_CBT_BIT_HELPER_GLSL
