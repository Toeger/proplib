#include "proplib/ui/label.h"
#include "proplib/ui/vertical_layout.h"
#include "proplib/utility/dependency_tracer.h"

#include <catch2/catch_all.hpp>

TEST_CASE("Apply width to reference children") {
	prop::Widget w1, w2;
	prop::Vertical_layout vl{&w1, &w2};
	//prop::print_status(vl.position);
	//prop::print_status(w1.position);
	//PROP_TRACER(vl).print_widget_trace(std::clog);
	PROP_TRACER(vl, w1, w2).to_image("/home/toeger/Projects/Prop/dot test/vl.png");
	vl.position.apply()->right = 100;
	//prop::print_status(vl.position);
	//prop::print_status(w1.position);
	//PROP_TRACER(vl).print_widget_trace(std::clog);
	REQUIRE(vl.position->right == 100);
	REQUIRE(w1.position->right == vl.position->right);
	REQUIRE(w2.position->right == vl.position->right);
}

TEST_CASE("Apply width to constructor children") {
	prop::Vertical_layout vl{
		prop::Label{{.text = "L1"}},
		prop::Label{{.text = "L2"}},
	};
	vl.position.apply()->right = vl.position.apply()->bottom = 100;
	REQUIRE(vl.position->right == 100);
	REQUIRE(vl.children[0]->position->right == vl.position->right);
	REQUIRE(vl.children[1]->position->right == vl.position->right);
}

TEST_CASE("Apply width to assigned children") {
	prop::Vertical_layout vl;
	vl.position.apply()->right = vl.position.apply()->bottom = 100;
	vl.children.apply()->push_back(prop::Label{{.text = "L1"}});
	vl.children.apply()->push_back(prop::Label{{.text = "L2"}});
	REQUIRE(vl.position->right == 100);
	REQUIRE(vl.children[0]->position->right == vl.position->right);
	REQUIRE(vl.children[1]->position->right == vl.position->right);
}
