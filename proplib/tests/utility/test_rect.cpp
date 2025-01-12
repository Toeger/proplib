#include "proplib/utility/rect.h"

#include <catch2/catch_all.hpp>

TEST_CASE("Aggregate construction") {
	constexpr prop::Rect rect{.top = 1, .left = 2, .bottom = 3, .right = 4};
	REQUIRE(rect.size() == prop::Size{rect.right - rect.left, rect.top - rect.bottom});

	[[maybe_unused]] constexpr prop::Size size{.width = 800, .height = 600};
}
