#pragma once

#include <cstdint>
#include <cstddef>

namespace CD {

constexpr std::size_t align(std::size_t offset, std::size_t alignment) {
	return (offset + (alignment - 1)) & ~(alignment - 1);
}

constexpr bool is_power_of_two(std::size_t n) {
	return n && (!(n & (n - 1)));
}

}