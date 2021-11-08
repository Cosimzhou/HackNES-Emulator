#pragma once
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#define LEN_ARRAY(a) (sizeof(a) / sizeof(a[0]))
#define TEST_BITS(var, bits) ((var & (bits)) == (bits))
#define CLR_BIT(var, bits) var &= ~(bits)
#define SET_BIT(var, bits) var |= (bits)
#define SWAP_BIT(var, vsrc, bits) \
  CLR_BIT(var, bits);             \
  SET_BIT(var, vsrc& bits)

#ifndef DEBUG
#define DDREPORT() throw "Report Error"
#define DDTRY() try {
#define DDCATCH() \
  }               \
  catch (...) {   \
    DebugDump();  \
    CHECK(0);     \
  }
#else
#define DDREPORT()
#define DDTRY()
#define DDCATCH()
#endif
namespace hn {
using Byte = std::uint8_t;
using Address = std::uint16_t;
using FileAddress = std::uint32_t;

template <typename T>
std::string DumpVector(const std::vector<T>& vec) {
  std::stringstream ss;
  ss << "[" << std::hex;
  int i = 0;
  for (auto c : vec) {
    if (i++ > 0) ss << ",";
    ss << static_cast<int>(c);
  }
  ss << "]";

  return ss.str();
}

}  // namespace hn
