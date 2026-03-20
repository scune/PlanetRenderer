#pragma once

#include <Libs.hpp>

#include <initializer_list>

template <class T>
class InitializerList
{
public:
  // Constructors
  inline constexpr InitializerList() noexcept : mFirst{nullptr}, mLast{nullptr}
  {
  }

  inline constexpr InitializerList(const T* data, uint32_t size) noexcept
      : mFirst{data}, mLast{data + size}
  {
  }

  inline constexpr InitializerList(const T* first, const T* last) noexcept
      : mFirst{first}, mLast{last}
  {
  }

  inline constexpr InitializerList(const InitializerList<T>& other) noexcept
      : mFirst{other.begin()}, mLast{other.end()}
  {
  }

  inline constexpr InitializerList(InitializerList<T>&& other) noexcept
      : mFirst{other.begin()}, mLast{other.end()}
  {
  }

  inline constexpr InitializerList(
      const std::initializer_list<T>& stdList) noexcept
      : mFirst{stdList.begin()}, mLast{stdList.end()}
  {
  }

  inline constexpr InitializerList(std::initializer_list<T>&& stdList) noexcept
      : mFirst{stdList.begin()}, mLast{stdList.end()}
  {
  }

  inline constexpr InitializerList(std::vector<T>&& vec) noexcept
      : mFirst{vec.data()}, mLast{vec.data() + vec.size()}
  {
  }

  inline constexpr InitializerList(const std::vector<T>& vec) noexcept
      : mFirst{vec.data()}, mLast{vec.data() + vec.size()}
  {
  }

  inline constexpr InitializerList(std::string&& str) noexcept
      : mFirst{str.data()}, mLast{str.data() + str.size()}
  {
  }

  inline constexpr InitializerList(const std::string& str) noexcept
      : mFirst{str.data()}, mLast{str.data() + str.size()}
  {
  }

  inline constexpr InitializerList(const T constArray[]) noexcept
      : mFirst{constArray}, mLast{constArray + sizeof(constArray)}
  {
  }

  // Get functions
  inline constexpr uint32_t size() const noexcept
  {
    return static_cast<uint32_t>(mLast - mFirst);
  }

  inline constexpr const T& operator[](uint32_t i) const noexcept
  {
    assert(i < size());
    return mFirst[i];
  }

  inline constexpr const T* begin() const noexcept { return mFirst; }
  inline constexpr const T* end() const noexcept { return mLast; }

private:
  const T* mFirst;
  const T* mLast;
};
