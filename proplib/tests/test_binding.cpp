#include "../utility/binding.h"

#include <catch2/catch.hpp>

TEST_CASE("Basic callback check") {
	int called{};
	prop::Property p = 42;
	REQUIRE(called == 0);
	prop::Binding b{[&p, &called] {
		p.get();
		called++;
	}};
	REQUIRE(called == 1);
	p = 0;
	REQUIRE(called == 2);
}
