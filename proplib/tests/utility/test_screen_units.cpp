#include "proplib/utility/screen_units.h"

#include <catch2/catch_all.hpp>

#if __GNUG__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#endif

TEST_CASE("Screen units Units") {
	using namespace prop::literals;
	prop::Pixels px;
	REQUIRE(px.amount == 0);
	REQUIRE(px == 0_px);
	px += 42_px;
	REQUIRE(px == 42_px);
	px -= 2_px;
	REQUIRE(px == 40_px);
	px /= 2;
	REQUIRE(px == 20_px);
	px *= 3;
	REQUIRE(px == 60_px);
	REQUIRE(6_px + 2_px == 8_px);
	REQUIRE(6_px - 2_px == 4_px);
	REQUIRE(6_px / 2_px == 3);
}

TEST_CASE("Screen units details") {
	using namespace prop;
	using namespace prop::detail;
	static constexpr Operator_set ops[] = {
		{Screen_dimension_type::xpos, Screen_dimension_type::width},
		{Screen_dimension_type::ypos, Screen_dimension_type::height},
		{Screen_dimension_type::width, Screen_dimension_type::width},
		{Screen_dimension_type::height, Screen_dimension_type::height},
	};
	static_assert(
		has_operator<Screen_dimension_type::xpos, Screen_dimension_type::width, Screen_dimension_type::xpos, ops>);
	static_assert(
		not has_operator<Screen_dimension_type::xpos, Screen_dimension_type::width, Screen_dimension_type::width, ops>);
	static_assert(std::is_same_v<Result_of<Screen_dimension_type::xpos, Screen_dimension_type::width, ops>, Xpos>);
	static_assert(not std::is_same_v<Result_of<Screen_dimension_type::xpos, Screen_dimension_type::width, ops>, Ypos>);
}

TEST_CASE("Screen units Dimensions") {
	using namespace prop::literals;
	prop::Xpos xpos;
	REQUIRE(xpos.amount == 0_px);
	REQUIRE(xpos == 0_px);
	xpos = 42_px;
	REQUIRE(xpos.amount == 42_px);
	REQUIRE(xpos == 42_px);
	REQUIRE(xpos - xpos == 0_px);
	REQUIRE(xpos - 42_px == 0_px);
}

#if __GNUG__
#pragma GCC diagnostic pop
#endif
