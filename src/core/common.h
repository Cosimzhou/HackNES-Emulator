#pragma once
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#define LEN_ARRAY(a) (sizeof(a) / sizeof(a[0]))
#define TEST_BITS(var, bits) ((var & (bits)) == (bits))
#define TEST_BIT(var, bits) ((var & (bits)) != 0)
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
using Word = std::uint16_t;
using Address = std::uint16_t;
using DWord = std::uint32_t;
using FileAddress = std::uint32_t;
using Color = Byte;
using RGBA = std::uint32_t;
using TimePoint = std::chrono::high_resolution_clock::time_point;
using Memory = std::vector<Byte>;
using Image = std::vector<std::vector<Color>>;

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

class Serialize {
 public:
  virtual void Save(std::ostream& os) = 0;
  virtual void Restore(std::istream& is) = 0;

 protected:
  template <typename T>
  void Write(std::ostream& os, T data) {
    os.write(reinterpret_cast<const char*>(&data), sizeof(T));
  }

  template <typename T>
  void Read(std::istream& is, T& data) {
    is.read(reinterpret_cast<char*>(&data), sizeof(T));
  }

  template <typename T>
  void Write(std::ostream& os, std::vector<T> data) {
    uint32_t size = static_cast<uint32_t>(data.size());
    os.write(reinterpret_cast<const char*>(&size), sizeof(size));
    os.write(reinterpret_cast<const char*>(data.data()), size * sizeof(T));
  }

  template <typename T>
  void Read(std::istream& is, std::vector<T>& data) {
    uint32_t size;
    is.read(reinterpret_cast<char*>(&size), sizeof(size));
    data.resize(static_cast<size_t>(size));
    is.read(reinterpret_cast<char*>(data.data()), size * sizeof(T));
  }

  void WriteNum(std::ostream& os, uint32_t data) {
    os.write(reinterpret_cast<const char*>(&data), sizeof(data));
  }

  uint32_t ReadNum(std::istream& is) {
    uint32_t data;
    is.read(reinterpret_cast<char*>(&data), sizeof(data));
    return data;
  }

  void WriteLNum(std::ostream& os, uint64_t data) {
    os.write(reinterpret_cast<const char*>(&data), sizeof(data));
  }

  uint64_t ReadLNum(std::istream& is) {
    uint64_t data;
    is.read(reinterpret_cast<char*>(&data), sizeof(data));
    return data;
  }
};

class DebugObject {
 public:
  virtual void DebugDump() = 0;
};

}  // namespace hn
