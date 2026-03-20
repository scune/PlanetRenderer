#pragma once

#include <Libs.hpp>

#include <exception>

static inline void ThrowErr(const char* mesg, int line, const char* file)
{
  throw std::runtime_error(std::string(mesg) + " On line " +
                           std::to_string(line) + " in file " +
                           std::string(file));
}

static inline void FuncIfNThrow(VkResult res, const char* mesg, int line,
                                const char* file)
{
  if (res != VK_SUCCESS)
  {
    std::string errStr = mesg;
    errStr += " With vulkan error:";
    errStr += string_VkResult(res);
    ThrowErr(errStr.c_str(), line, file);
  }
}

static inline void FuncIfNThrow(bool res, const char* mesg, int line,
                                const char* file)
{
  if (!res)
  {
    ThrowErr(mesg, line, file);
  }
}

static inline void FuncIfNThrow(const void* ptr, const char* mesg, int line,
                                const char* file)
{
  if (!ptr)
  {
    ThrowErr(mesg, line, file);
  }
}

#define IfNThrow(res, mesg)                                                    \
  {                                                                            \
    FuncIfNThrow(res, mesg, __LINE__, __FILE__);                               \
  }

static inline bool DetermineRes(VkResult res) { return res != VK_SUCCESS; }
static inline bool DetermineRes(bool res) { return !res; }

#define IfNRetF(res)                                                           \
  if (DetermineRes(res))                                                       \
  {                                                                            \
    return false;                                                              \
  }

static inline bool FuncIfNRetFM(VkResult res, const char* mesg, int line,
                                const char* file)
{
  if (res != VK_SUCCESS)
  {
    std::cerr << "\033[33m[RETURNED FALSE]: Line " << line << " file " << file
              << " with message: " << mesg << " and vulkan error "
              << string_VkResult(res) << "\033[0m" << std::endl;
    return true;
  }
  return false;
}

static inline bool FuncIfNRetFM(bool res, const char* mesg, int line,
                                const char* file)
{
  if (!res)
  {
    std::cerr << "\033[33m[RETURNED FALSE]: Line " << line << " file " << file
              << " with message: " << mesg << "\033[0m" << std::endl;
    return true;
  }
  return false;
}

static inline bool FuncIfNRetFM(const void* ptr, const char* mesg, int line,
                                const char* file)
{
  if (!ptr)
  {
    std::cerr << "\033[33m[RETURNED FALSE]: Line " << line << " file " << file
              << " with message: " << mesg << "\033[0m" << std::endl;
    return true;
  }
  return false;
}

#define IfNRetFM(res, mesg)                                                    \
  if (FuncIfNRetFM(res, mesg, __LINE__, __FILE__))                             \
  {                                                                            \
    return false;                                                              \
  }

static inline bool PrintWarningFunc(bool res, const char* mesg, int line,
                                    const char* file)
{
  if (!res)
  {
    std::cerr << "\033[33m[WARNING]: Line " << line << " file " << file
              << " with message: " << mesg << "\033[0m" << std::endl;
  }
  return !res;
}

#define PrintWarning(res, mesg) PrintWarningFunc(res, mesg, __LINE__, __FILE__)

void PrintErr(const std::exception& exception) noexcept;
