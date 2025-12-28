#include "prop/utility/property.h"
#include "prop/utility/tracking_list.h"

#include <catch2/catch_all.hpp>

TEST_CASE("Dependency status", "[Tracking_list]") {
	prop::Property p1 = 42;
	prop::Property p2 = 3.14;
	auto p1_status = p1.get_status();
	auto p2_status = p2.get_status();
	{
		auto tl = prop::Tracking_list<>::of(p1, p2);
		REQUIRE(tl.is_dependent_on(p1));
		REQUIRE(tl.is_dependent_on(p2));
		REQUIRE(p1_status != p1.get_status());
		REQUIRE(p2_status != p2.get_status());
	}
	REQUIRE(p1_status == p1.get_status());
	REQUIRE(p2_status == p2.get_status());
}

TEST_CASE("Expiration and move detection", "[Tracking_list]") {
	prop::Property p0 = 42;
	prop::Property p1 = 314;
	auto tl = prop::Tracking_list<>::of(p0, p1);
	REQUIRE(tl[0] == &p0);
	REQUIRE(tl[1] == &p1);
	auto{std::move(p0)};
	REQUIRE(tl[0] == nullptr);
	REQUIRE(tl[1] == &p1);
	p0 = std::move(p1);
	REQUIRE(tl[0] == nullptr);
	REQUIRE(tl[1] == &p0);
	auto{std::move(p0)};
	REQUIRE(tl[0] == nullptr);
	REQUIRE(tl[1] == nullptr);
}

TEST_CASE("Static 'of' functions", "[Tracking_list]") {
	prop::Property explicit_dependency = 1;
	prop::Property implicit_dependency = 1;
	prop::Property<int> test_property{[&implicit_dependency](int exp) { return implicit_dependency.get() + exp; },
									  explicit_dependency};
	prop::Property dependent = [&test_property] { return test_property; };
}
