#include "proplib/ui/widget.h"
#include "proplib/utility/dependency_tracer.h"

#include <catch2/catch_all.hpp>

TEST_CASE("Selfie test") {
	prop::Widget w;
	auto selfie = w.selfie();
	REQUIRE(selfie == &w);
	{
		PROP_TRACER(selfie, w).to_image("/home/toeger/Projects/Prop/dot test/vl.png");
		prop::Widget w2 = std::move(w);
		PROP_TRACER(selfie, w, w2).to_image("/home/toeger/Projects/Prop/dot test/vl.png");
		REQUIRE(selfie == &w2);
	}
	REQUIRE(selfie == nullptr);
}
