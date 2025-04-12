#include "proplib/ui/label.h"

#include <catch2/catch_all.hpp>

TEST_CASE("Move test") {
	prop::Label label;
	label.text = "2";
	label.position.apply()->top = 3;
	label.position.apply()->left = 4;
	label.position.apply()->bottom = 7;
	label.position.apply()->right = 8;

	REQUIRE(label.text == "2");
	REQUIRE(label.position->top == 3);
	REQUIRE(label.position->left == 4);
	REQUIRE(label.position->bottom == 7);
	REQUIRE(label.position->right == 8);
	REQUIRE(label.self == label);

	auto l2 = std::move(label);
	REQUIRE(l2.text == "2");
	REQUIRE(l2.position->top == 3);
	REQUIRE(l2.position->left == 4);
	REQUIRE(l2.position->bottom == 7);
	REQUIRE(l2.position->right == 8);
	REQUIRE(l2.self == l2);

	std::swap(label, l2);
	REQUIRE(label.text == "2");
	REQUIRE(label.position->top == 3);
	REQUIRE(label.position->left == 4);
	REQUIRE(label.position->bottom == 7);
	REQUIRE(label.position->right == 8);
	REQUIRE(label.self == label);
}
