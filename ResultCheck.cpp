#include "ResultCheck.hpp"

void PrintErr(const std::exception& exception) noexcept
{
  if (exception.what())
  {
    std::cerr << "\033[31m[EXCEPTION THROWN] With message: " << exception.what()
              << "\033[0m" << std::endl;
  }
  else
  {
    std::cerr << "\033[31m[EXCEPTION THROWN]: Unknown!\033[0m" << std::endl;
  }
}
