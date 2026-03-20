#pragma once

// Vulkan
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>
#include <wayland-client-protocol.h>

// Glfw
#define GLFW_EXPOSE_NATIVE_WAYLAND
#define GLFW_INCLUDE_VULKAN
#include <glfw3.h>
#include <glfw3native.h>

// Glm
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/component_wise.hpp>

// Std
#include <array>
#include <assert.h>
#include <cstdint>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#if !defined(NDEBUG) || defined(_DEBUG)
#define DEBUG
#endif

template <typename T>
static inline size_t VecByteSize(const std::vector<T>& vec) noexcept
{
  return vec.size() * sizeof(typename std::vector<T>::value_type);
}

template <typename T>
static inline size_t VecCapByteSize(const std::vector<T>& vec) noexcept
{
  return vec.capacity() * sizeof(typename std::vector<T>::value_type);
}

#define COUT(str__) std::cout << str__ << "\n"
#define COUT_VEC3(v__)                                                         \
  std::cout << v__[0] << "x " << v__[1] << "y " << v__[2] << "z\n"
#define COUT_VEC4(v__)                                                         \
  std::cout << v__[0] << "x " << v__[1] << "y " << v__[2] << "z " << v__[3]    \
            << "w\n"
#define COUT_MAT3(m__)                                                         \
  for (uint32_t i__ = 0; i__ < 3; i__++)                                       \
  {                                                                            \
    COUT_VEC3(m__[i__]);                                                       \
  }
#define COUT_MAT4(m__)                                                         \
  for (uint32_t i__ = 0; i__ < 3; i__++)                                       \
  {                                                                            \
    COUT_VEC4(m__[i__]);                                                       \
  }
