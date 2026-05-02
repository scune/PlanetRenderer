#pragma once

#include "Libs.hpp"

inline void FastPrint(const std::string& str)
{
  fwrite(str.data(), str.size(), 1, stdout);
}
