#include "proplib/ui/widget.h"
#include "proplib/utility/dependency_tracer.h"

#include <catch2/catch_all.hpp>

TEST_CASE("Tracking test") {
	prop::Widget w;
	auto ptr = prop::track(w);
	REQUIRE(ptr == &w);
	{
		PROP_TRACER(ptr, w).to_image("/home/toeger/Projects/Prop/dot test/vl.png");
		ptr.print_status();
		prop::Widget w2 = std::move(w);
		ptr.print_status();
		PROP_TRACER(ptr, w, w2).to_image("/home/toeger/Projects/Prop/dot test/vl.png");
		REQUIRE(ptr == &w2);
	}
	REQUIRE(ptr == nullptr);
}
