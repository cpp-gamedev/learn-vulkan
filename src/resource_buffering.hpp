#pragma once
#include <array>

namespace lvk {
// Number of virtual frames.
inline constexpr std::size_t resource_buffering_v{2};

// Alias for N-buffered resources.
template <typename Type>
using Buffered = std::array<Type, resource_buffering_v>;
} // namespace lvk
