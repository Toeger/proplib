#include "internals/vertical_layout.privates.h"
#include "ui/vertical_layout.h"

#include <catch2/catch_all.hpp>

TEST_CASE("Apply width to children") {
	prop::Widget w1, w2;
	prop::Vertical_layout vl{&w1, &w2};
	prop::print_status(vl.width);
	prop::print_status(vl.privates->layout_updater);
	vl.width = vl.height = 100;
	REQUIRE(w1.width == vl.width);
	REQUIRE(w2.width == vl.width);
}
