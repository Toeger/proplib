#include <catch2/catch.hpp>

#include "../utility/polywrap.h"

TEST_CASE("Constructor") {
	static_assert(!prop::detail::is_dereferenceable_v<int>);
	static_assert(std::is_same_v<std::remove_cvref_t<int>, int>);
	static_assert(prop::detail::is_dereferenceable_v<int *>);
	static_assert(prop::detail::is_dereferenceable_v<std::unique_ptr<int>>);
	REQUIRE(*prop::Polywrap(42) == 42);
	REQUIRE(*prop::Polywrap(std::make_unique<int>(42)) == 42);
}

TEST_CASE("Value types") {
	prop::Polywrap<int> p;
	REQUIRE_FALSE(p);
	p = 42;
	REQUIRE(p);
	REQUIRE(*p.get() == 42);
}

TEST_CASE("Pointer types") {
	prop::Polywrap<int> p;
	REQUIRE_FALSE(p);
	bool sp_deleted = false;
	std::shared_ptr<int> sp{new int(24), [&sp_deleted](int *i) {
								sp_deleted = true;
								delete i;
							}};
	p = std::move(sp);
	REQUIRE_FALSE(sp);
	REQUIRE(p);
	REQUIRE_FALSE(sp_deleted);
	p = nullptr;
	REQUIRE(sp_deleted);
}

TEST_CASE("Assigning Polywraps") {
	prop::Polywrap p = 42;
	prop::Polywrap p2 = p;
	static_assert(std::is_same_v<decltype(*p2.get()), int &>);
}

TEST_CASE("Moving Polywraps") {
	prop::Polywrap p = 42;
	prop::Polywrap p2 = std::move(p);
	REQUIRE_FALSE(p);
}
