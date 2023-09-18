#ifndef LAND15_COMMON_RANDOM_H_
#define LAND15_COMMON_RANDOM_H_

#include <stdint.h>

namespace land15 {
namespace common {

// Produces a pseudo-random 64bit int. Repeated calls to this PRNG will almost
// always produce a unique stream of values per-thread.
uint64_t rnd();

// Produces a pseudo-random double in the range `[0, 1)`. Uses rnd().
double rndd();

// Produces a pseudo-random double in the range `[start_inc, end_ex)`. Uses
// `rnd()`.
double rndd(double start_inc, double end_ex);

// Provide a specific seed to the PRNG in the current thread. A given seed will
// always produce the same stream of random values.
void srnd(uint64_t s);

}  // namespace common
}  // namespace chime

#endif  // LAND15_COMMON_RANDOM_H_
