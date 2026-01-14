#include "prop/ui/label.h"
#include "prop/ui/vertical_layout.h"
#include "prop/utility/dependency_tracer.h"

#include <catch2/catch_all.hpp>

TEST_CASE("Apply width to reference children", "[Vertical_layout]") {
	prop::Widget w1, w2;
	w1.set_name("w1");
	w2.set_name("w2");
	prop::Vertical_layout vl{&w1, &w2};
	vl.set_name("vl");
	PROP_TRACER(vl, w1, w2).to_image();
	vl.position.apply()->right = 100;
	REQUIRE(vl.position->right == 100);
	REQUIRE(w1.position->right == vl.position->right);
	REQUIRE(w2.position->right == vl.position->right);
}

TEST_CASE("Apply width to constructor children", "[Vertical_layout]") {
	prop::Vertical_layout vl{
		prop::Label{{.text = "L1"}},
		prop::Label{{.text = "L2"}},
	};
	vl.position.apply()->right = vl.position.apply()->bottom = 100;
	REQUIRE(vl.position->right == 100);
	REQUIRE(vl.children[0]->position->right == vl.position->right);
	REQUIRE(vl.children[1]->position->right == vl.position->right);
}

TEST_CASE("Apply width to assigned children", "[Vertical_layout]") {
	prop::Vertical_layout vl;
	vl.position.apply()->right = vl.position.apply()->bottom = 100;
	vl.children.apply()->push_back(prop::Label{{.text = "L1"}});
	vl.children.apply()->push_back(prop::Label{{.text = "L2"}});
	REQUIRE(vl.position->right == 100);
	REQUIRE(vl.children[0]->position->right == vl.position->right);
	REQUIRE(vl.children[1]->position->right == vl.position->right);
}
