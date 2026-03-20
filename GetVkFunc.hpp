#pragma once

#include "ResultCheck.hpp"

// clang-format off
#define GetVkFuncAuto(instance, name)                                          \
  auto name = (PFN_##name)vkGetInstanceProcAddr(instance, #name);              \
  IfNThrow(name, "Failed to get function pointer for " #name "!");

#define GetVkFunc(instance, name)                                              \
  (PFN_##name)vkGetInstanceProcAddr(instance, #name);                          \
  IfNThrow(name, "Failed to get function pointer for " #name "!");
// clang-format on
