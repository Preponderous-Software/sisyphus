// splitmix64: small, fast, the same on every platform.
#pragma once

#include <cstdint>

namespace sisyphus {

inline uint64_t nextRandom(uint64_t& state) {
    uint64_t z = (state += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

// Uniform in [0, 1), from the top 24 bits (exact in a float).
inline float randomUnit(uint64_t& state) { return static_cast<float>(nextRandom(state) >> 40) / 16777216.0f; }

inline int randomBelow(uint64_t& state, int n) { return static_cast<int>(nextRandom(state) % static_cast<uint64_t>(n)); }

}  // namespace sisyphus
