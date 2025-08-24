#include "proplib/platform/platform.h"
#include "proplib/utility/screen_units.h"

#include <catch2/catch_all.hpp>
#include <iostream>
#include <print>

#if __GNUG__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wdouble-promotion"
#endif

using namespace prop::literals;

TEST_CASE("Screen units Units") {
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
	static constexpr auto ops =
		std::to_array<Operator_set>({{Screen_dimension_type::xpos, Screen_dimension_type::width},
									 {Screen_dimension_type::ypos, Screen_dimension_type::height},
									 {Screen_dimension_type::width, Screen_dimension_type::width},
									 {Screen_dimension_type::height, Screen_dimension_type::height}});
	static_assert(
		has_operator<Screen_dimension_type::xpos, Screen_dimension_type::width, Screen_dimension_type::xpos, ops>);
	static_assert(
		not has_operator<Screen_dimension_type::xpos, Screen_dimension_type::width, Screen_dimension_type::width, ops>);
	static_assert(std::is_same_v<Result_of<Screen_dimension_type::xpos, Screen_dimension_type::width, ops>, Xpos>);
	static_assert(not std::is_same_v<Result_of<Screen_dimension_type::xpos, Screen_dimension_type::width, ops>, Ypos>);
}

TEST_CASE("Screen units Dimensions") {
	prop::Xpos xpos;
	REQUIRE(xpos.amount == 0_px);
	REQUIRE(xpos == 0_px);
	xpos = 42_px;
	REQUIRE(xpos.amount == 42_px);
	REQUIRE(xpos == 42_px);
	REQUIRE(xpos - xpos == 0_px);
	REQUIRE(xpos - 42_px == 0_px);
}

TEST_CASE("Screen dimensions") {
	int screen_count{};
	for (auto &screen : prop::platform::get_screens()) {
		std::print("Screen {} with resolution {}x{} and size {}x{}mm\n", ++screen_count, screen.width_pixels,
				   screen.height_pixels, screen.width_mm, screen.height_mm);
	}
	std::cout << "1xmm = " << prop::Pixels{1_xmm} << "\n1ymm = " << prop::Pixels{1_ymm} << "\n";
	std::cout << prop::X_millimeters{1_px} << 'x' << prop::Y_millimeters{1_px} << '\n';
	std::println("1xmm = {}\n1ymm = {}", prop::Pixels{1_xmm}, prop::Pixels{1_ymm});
	std::println("1pt = {}", prop::Pixels{1_pt});
}

#if __GNUG__
#pragma GCC diagnostic pop
#endif
