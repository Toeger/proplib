#include "property.h"
#include <catch2/catch.hpp>

TEST_CASE("Basic Property tests") {
	pro::Property p = 42;
	REQUIRE(p == 42);
	p = [] { return 12; };
	REQUIRE(p == 12);
	pro::Property p2 = [&p] { return p + 3; };
	REQUIRE(p2 == 15);
	p = 1;
	REQUIRE(p2 == 4);

	pro::Property<int> p3 = [&] {
		if (p) {
			return +p;
		}
		return +p2;
	};

	REQUIRE(p3 == 1);
}
