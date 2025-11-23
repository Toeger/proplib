#include "proplib/utility/property.h"
#include "proplib/utility/tracking_pointer.h"
#include <catch2/catch_all.hpp>

TEST_CASE("Basic tracking") {
	std::cout << std::flush;
	prop::Property pi{42};
	pi.print_status();
	auto tppi = prop::track(&pi);
	pi.print_status();
	tppi.print_status();
	REQUIRE(tppi == &pi);
	auto pi2 = std::move(pi);
	REQUIRE(tppi == &pi2);
	auto{std::move(pi2)};
	tppi.print_status();
	REQUIRE(tppi == nullptr);
}
