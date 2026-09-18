## 2026-09-18 - Linear search vs binary search for small lookup grids
**Learning:** For small fixed-size lookup tables ($N \le 16$), linear scan significantly outperforms binary search due to branch predictability, loop unrolling, and avoiding division/middle-index calculation overhead in CPU-bound interpolation loops.
**Action:** Use `if constexpr (N <= 16)` linear scan in `find_index_and_weight` for `std::array` lookup tables in physics/aerodynamics interpolation engines.
