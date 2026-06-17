#include <iostream>

#include "solution.hpp"
#include "solution_flat.hpp"

bool original_solution(MatrixOfDoubles& in, MatrixOfDoubles& out) {
    auto size = in.size();
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            out[i][j] = in[j][i];
        }
    }

    return out[0][size - 1];
}

bool matrices_equal(MatrixOfDoubles& m1, MatrixOfDoubles& m2) {
    if (m1.size() != m2.size()) {
        return false;
    }

    auto size = m1.size();
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            if (m1[i][j] != m2[i][j]) {
                return false;
            }
        }
    }

    return true;
}

bool flat_matrices_equal(FlatMatrix& m1, FlatMatrix& m2, int N) {
    for (int i = 0; i < N * N; i++) {
        if (m1[i] != m2[i]) {
            return false;
        }
    }
    return true;
}

int main() {
    constexpr int N = 2001;

    // Validate vector-of-vector version
    MatrixOfDoubles in;
    MatrixOfDoubles out;
    MatrixOfDoubles out_golden;

    in.resize(N, std::vector<double>(N, 0.0));
    out.resize(N, std::vector<double>(N, 0.0));
    out_golden.resize(N, std::vector<double>(N, 0.0));

    initMatrix(in);

    original_solution(in, out_golden);
    solution(in, out);

    if (!matrices_equal(out, out_golden)) {
        std::cerr << "Validation Failed (vector-of-vector)\n";
        return 1;
    }

    std::cout << "Validation Successful (vector-of-vector)\n";

    // Validate flat array version
    FlatMatrix in_flat(N * N, 0.0);
    FlatMatrix out_flat(N * N, 0.0);
    FlatMatrix out_golden_flat(N * N, 0.0);

    initFlatMatrix(in_flat, N);

    // Golden: simple transpose on flat array
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            out_golden_flat[i * N + j] = in_flat[j * N + i];
        }
    }

    solution_flat(in_flat, out_flat, N);

    if (!flat_matrices_equal(out_flat, out_golden_flat, N)) {
        std::cerr << "Validation Failed (flat array)\n";
        return 1;
    }

    std::cout << "Validation Successful (flat array)\n";
    return 0;
}
