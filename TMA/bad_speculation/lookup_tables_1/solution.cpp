#include "solution.hpp"

// Lookup table: replace 7-branch if/else chain with a direct array access.
// No branches → no branch mispredictions.
static const int buckets[100] = {
//  0  1  2  3  4  5  6  7  8  9
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  // 0-9
    0, 0, 0,                          // 10-12
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1,  // 13-22
    1, 1, 1, 1, 1, 1,                // 23-28
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  // 29-38
    2, 2,                            // 39-40
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3,  // 41-50
    3, 3,                            // 51-52
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4,  // 53-62
    4, 4, 4, 4, 4, 4, 4, 4,        // 63-70
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5,  // 71-80
    5, 5,                            // 81-82
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6,  // 83-92
    6, 6, 6, 6, 6, 6, 6             // 93-99
};

static std::size_t mapToBucket(std::size_t v) {
  if (v < sizeof(buckets) / sizeof(buckets[0]))
    return buckets[v];
  return DEFAULT_BUCKET;
}

std::array<std::size_t, NUM_BUCKETS> histogram(const std::vector<int> &values) {
  std::array<std::size_t, NUM_BUCKETS> retBuckets{0};
  for (auto v : values) {
    retBuckets[mapToBucket(v)]++;
  }
  return retBuckets;
}
