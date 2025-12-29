#include "prop/utility/required_pointer.h"

#include <catch2/catch_all.hpp>

TEST_CASE("Basic value", "[Required_pointer]") {
	int i{};
	prop::Required_pointer<int> ri{&i, true};
	REQUIRE(ri == &i);
	REQUIRE(ri.is_required());
	REQUIRE(ri.get_pointer() == &i);
	int j{};
	ri = &j;
	REQUIRE(ri == &j);
	REQUIRE(ri.is_required());
	REQUIRE(ri.get_pointer() == &j);
	ri.set_required(false);
	REQUIRE(not ri.is_required());
	REQUIRE(ri == &j);
	auto ri2 = ri;
	REQUIRE(ri2 == &j);
	REQUIRE(not ri2.is_required());
	REQUIRE(ri2.get_pointer() == &j);
	ri2.set_required(true);
	REQUIRE(ri2 == &j);
	REQUIRE(ri2.is_required());
	REQUIRE(not ri.is_required());
}
