#include "solution.hpp"

uint16_t checksum(const Blob &blob) {
  // Widen accumulator to uint32_t: removes per-iteration carry check,
  // allowing the compiler to auto-vectorize the loop.
  uint32_t acc = 0;
  for (const auto value : blob) {
    acc += value;
  }

  // Final carry correction (at most 1 carry since 64K × 65535 < 2^32)
  auto high = acc >> 16;
  auto low = acc & 0xFFFFu;
  acc = low + high;

  // Account for potential overflow from the first correction
  high = acc >> 16;
  low = acc & 0xFFFFu;
  acc = low + high;

  return static_cast<uint16_t>(acc);
}
