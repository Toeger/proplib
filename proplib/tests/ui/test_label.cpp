#include "ui/label.h"

#include <catch2/catch_all.hpp>

TEST_CASE("Move test") {
	prop::Label label;
	label.font_size = 1;
	label.text = "2";
	label.height = 3;
	label.width = 4;
	label.preferred_height = 5;
	label.preferred_width = 6;
	label.x = 7;
	label.y = 8;

	REQUIRE(label.font_size == 1);
	REQUIRE(label.text == "2");
	REQUIRE(label.height == 3);
	REQUIRE(label.width == 4);
	REQUIRE(label.preferred_height == 5);
	REQUIRE(label.preferred_width == 6);
	REQUIRE(label.x == 7);
	REQUIRE(label.y == 8);

	auto l2 = std::move(label);
	REQUIRE(l2.font_size == 1);
	REQUIRE(l2.text == "2");
	REQUIRE(l2.height == 3);
	REQUIRE(l2.width == 4);
	REQUIRE(l2.preferred_height == 5);
	REQUIRE(l2.preferred_width == 6);
	REQUIRE(l2.x == 7);
	REQUIRE(l2.y == 8);
}
