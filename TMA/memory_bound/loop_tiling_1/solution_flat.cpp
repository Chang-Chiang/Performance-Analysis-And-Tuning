#include "solution_flat.hpp"

#include <algorithm>

void initFlatMatrix(FlatMatrix& m, int N) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            m[i * N + j] = (i + j) % 1024;
        }
    }
}

bool solution_flat(FlatMatrix& in, FlatMatrix& out, int N) {
    static constexpr int TILE_SIZE = 16;

    for (int ii = 0; ii < N; ii += TILE_SIZE) {
        for (int jj = 0; jj < N; jj += TILE_SIZE) {
            for (int i = ii; i < std::min(ii + TILE_SIZE, N); i++) {
                for (int j = jj; j < std::min(jj + TILE_SIZE, N); j++) {
                    out[i * N + j] = in[j * N + i];
                }
            }
        }
    }
    return out[0 * N + (N - 1)];
}
