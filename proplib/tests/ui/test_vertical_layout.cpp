#include "proplib/internals/vertical_layout.privates.h"
#include "proplib/ui/button.h"
#include "proplib/ui/label.h"
#include "proplib/ui/vertical_layout.h"
#include "proplib/ui/window.h"

#include <catch2/catch_all.hpp>

TEST_CASE("Apply width to children") {
	prop::Widget w1, w2;
	prop::Vertical_layout vl{&w1, &w2};
	vl.width = vl.height = 100;
	REQUIRE(w1.width == vl.width);
	REQUIRE(w2.width == vl.width);
}

TEST_CASE("Apply width to children2") {
	prop::Vertical_layout vl{
		prop::Label{{
			.text = "L1",
		}},
		prop::Label{{
			.text = "L2",
		}},
	};
	vl.width = vl.height = 100;
	auto &w1 = vl.children.get()[0];
	auto &w2 = vl.children.get()[1];
	REQUIRE(w1->width == vl.width);
	REQUIRE(w2->width == vl.width);
}

TEST_CASE("Apply width to children3") {
	prop::Window w{{
		.title = "Prop Test",
	}};
	w.widget = prop::Vertical_layout{
		prop::Label{{
			.text = "Hello world!",
		}},
		prop::Label{{
			.text = "Automatic layouting!!!",
		}},
		prop::Label{{
			.text = "So cool!!!",
		}},
		prop::Button{{
			.text = "Clickable too!!!",
			.callback = [] { std::cout << "Button clicked\n"; },
		}},
	};
	w.widget.apply()->get()->width = 700;
	w.widget.apply([](auto &w) { w->width = w->height = 100; });
	//auto &w1 = static_cast<const prop::Vertical_layout *>(w.widget.get().get())->children.get()[0];
	//auto &w2 = static_cast<const prop::Vertical_layout *>(w.widget.get().get())->children.get()[1];
	//REQUIRE(w1->width == w.widget.get()->width);
	//REQUIRE(w2->width == w.widget.get()->width);
}
