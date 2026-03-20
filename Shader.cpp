#include "Shader.hpp"

#include "ResultCheck.hpp"
#include <fstream>

void Shader::LoadSPV(const char* name, std::vector<uint32_t>& shaderCode)
{
  char path[256] = "Shaders/Compiled/";
  assert(strlen(path) + strlen(name) < 256);
  strcat(path, name);

  std::ifstream file(path, std::ios::ate | std::ios::binary);
  IfNThrow(file.is_open(),
           (std::string("Failed to open shader file ") + path + "!").c_str());

  const size_t fileSize = (size_t)file.tellg();

  file.seekg(0);

  shaderCode.resize(fileSize / sizeof(uint32_t));
  file.read(reinterpret_cast<char*>(shaderCode.data()), fileSize);

  file.close();
}
