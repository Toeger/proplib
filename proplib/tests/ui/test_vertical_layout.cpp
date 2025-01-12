#include "proplib/ui/label.h"
#include "proplib/ui/vertical_layout.h"

#include <catch2/catch_all.hpp>

TEST_CASE("Apply width to reference children") {
	std::cerr << "------------ " << __LINE__ << '\n';
	prop::Widget w1, w2;
	std::cerr << "------------ " << __LINE__ << '\n';
	prop::Vertical_layout vl{&w1, &w2};
	std::cerr << "------------ " << __LINE__ << '\n';
	prop::print_status(vl.rect);
	vl.rect.apply()->right = 100;
	std::cerr << "------------ " << __LINE__ << '\n';
	vl.rect.apply()->left = 100;
	std::cerr << "------------ " << __LINE__ << '\n';
	REQUIRE(vl.rect->right == 100);
	REQUIRE(w1.rect->right == vl.rect->right);
	REQUIRE(w2.rect->right == vl.rect->right);
}

TEST_CASE("Apply width to constructor children") {
	prop::Vertical_layout vl{
		prop::Label{{.text = "L1"}},
		prop::Label{{.text = "L2"}},
	};
	vl.rect.apply()->right = vl.rect.apply()->bottom = 100;
	REQUIRE(vl.rect->right == 100);
	REQUIRE(vl.children[0]->rect->right == vl.rect->right);
	REQUIRE(vl.children[1]->rect->right == vl.rect->right);
}

TEST_CASE("Apply width to assigned children") {
	prop::Vertical_layout vl;
	vl.rect.apply()->right = vl.rect.apply()->bottom = 100;
	vl.children.apply()->push_back(prop::Label{{.text = "L1"}});
	vl.children.apply()->push_back(prop::Label{{.text = "L2"}});
	REQUIRE(vl.rect->right == 100);
	REQUIRE(vl.children[0]->rect->right == vl.rect->right);
	REQUIRE(vl.children[1]->rect->right == vl.rect->right);
}
