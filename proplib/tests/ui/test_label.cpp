#include "proplib/ui/label.h"

#include <catch2/catch_all.hpp>

TEST_CASE("Move test") {
	prop::Label label;
	label.text = "2";
	label.rect.apply()->top = 3;
	label.rect.apply()->left = 4;
	label.rect.apply()->bottom = 7;
	label.rect.apply()->right = 8;

	REQUIRE(label.text == "2");
	REQUIRE(label.rect->top == 3);
	REQUIRE(label.rect->left == 4);
	REQUIRE(label.rect->bottom == 7);
	REQUIRE(label.rect->right == 8);
	REQUIRE(label.self == label);

	auto l2 = std::move(label);
	REQUIRE(l2.text == "2");
	REQUIRE(l2.rect->top == 3);
	REQUIRE(l2.rect->left == 4);
	REQUIRE(l2.rect->bottom == 7);
	REQUIRE(l2.rect->right == 8);
	REQUIRE(l2.self == l2);

	std::swap(label, l2);
	REQUIRE(label.text == "2");
	REQUIRE(label.rect->top == 3);
	REQUIRE(label.rect->left == 4);
	REQUIRE(label.rect->bottom == 7);
	REQUIRE(label.rect->right == 8);
	REQUIRE(label.self == label);
}
