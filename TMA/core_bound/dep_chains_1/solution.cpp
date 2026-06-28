#include "solution.hpp"
#include <array>
#include <iostream>

unsigned getSumOfDigits(unsigned n) {
  unsigned sum = 0;
  while (n != 0) {
    sum = sum + n % 10;
    n = n / 10;
  }
  return sum;
}

// Optimization: process M elements from l1 at once, traversing l2 only once
// for all M. This amortizes the l2 pointer-chasing dependency chain.
template <int M>
unsigned solutionM(List *l1, List *l2) {
  unsigned retVal = 0;
  List *head2 = l2;
  List *head1 = l1;

  // Count l1 length
  int length1 = 0;
  while (l1) {
    length1++;
    l1 = l1->next;
  }

  l1 = head1;

  // Process M elements from l1 at a time
  for (int i = 0; i < length1 / M; i++) {
    std::array<unsigned, M> vals;
    // Read M values from l1
    for (int j = 0; j < M; j++) {
      vals[j] = l1->value;
      l1 = l1->next;
    }
    // Traverse l2 once, checking all M values
    l2 = head2;
    int found = 0;
    while (l2) {
      for (int j = 0; j < M; j++) {
        if (l2->value == vals[j]) {
          retVal += getSumOfDigits(l2->value);
          if (++found == M)
            break;
        }
      }
      if (found == M)
        break;
      l2 = l2->next;
    }
  }

  // Remainder: sequential
  while (l1) {
    unsigned v = l1->value;
    l2 = head2;
    while (l2) {
      if (l2->value == v) {
        retVal += getSumOfDigits(v);
        break;
      }
      l2 = l2->next;
    }
    l1 = l1->next;
  }

  return retVal;
}

unsigned solution(List *l1, List *l2) {
  return solutionM<4>(l1, l2);
}
