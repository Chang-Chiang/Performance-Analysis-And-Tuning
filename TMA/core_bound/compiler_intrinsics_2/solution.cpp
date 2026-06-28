#include "solution.hpp"
#include <immintrin.h>  // AVX2
#include <algorithm>
#include <cstdint>

// Find the longest line in a file using AVX2 intrinsics.
// Processes 32 bytes per iteration, using SIMD to find '\n' characters.
uint32_t solution(const std::string &inputContents) {
    uint32_t longestLine = 0;
    uint32_t curLineLength = 0;
    uint32_t pos = 0;
    auto strLength = inputContents.size();

    if (strLength >= 32) {
        const __m256i eol = _mm256_set1_epi8('\n');
        auto *buffer = inputContents.data();
        uint32_t curLineBegin = 0;

        for (; pos + 32 < strLength; pos += 32) {
            // Load 32 bytes and compare with '\n'
            __m256i vect = _mm256_loadu_si256((const __m256i *)buffer);
            __m256i vectMask = _mm256_cmpeq_epi8(vect, eol);
            uint32_t mask = _mm256_movemask_epi8(vectMask);

            while (mask) {
                int maskPos = __builtin_ctz(mask);  // trailing '1' bit position

                uint32_t curLen = (pos - curLineBegin) + maskPos;
                if (pos < curLineBegin)
                    curLen = maskPos;

                curLineBegin += curLen + 1;
                longestLine = std::max(curLen, longestLine);

                ++maskPos;
                if (maskPos > 31)
                    break;
                else
                    mask >>= maskPos;
            }
            buffer += 32;
        }
        curLineLength = pos - curLineBegin;
    }

    // Remainder: scalar loop
    for (; pos < strLength; pos++) {
        curLineLength = (inputContents[pos] == '\n') ? 0 : curLineLength + 1;
        longestLine = std::max(curLineLength, longestLine);
    }

    return longestLine;
}
