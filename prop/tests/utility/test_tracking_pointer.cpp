#include "prop/utility/property.h"
#include "prop/utility/tracking_pointer.h"
#include <catch2/catch_all.hpp>

TEST_CASE("Basic tracking") {
	prop::Property pi{42};
	INFO(pi.get_status());
	auto tppi = prop::track(&pi);
	INFO(pi.get_status());
	INFO(tppi.get_status());
	REQUIRE(tppi == &pi);
	auto pi2 = std::move(pi);
	REQUIRE(tppi == &pi2);
	auto{std::move(pi2)};
	INFO(tppi.get_status());
	REQUIRE(tppi == nullptr);
}
