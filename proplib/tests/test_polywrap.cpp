#include "../utility/polywrap.h"
#include "../utility/type_traits.h"

#include <catch2/catch.hpp>
#include <functional>

TEST_CASE("Concepts") {
	struct Base {};
	struct Derived : Base {};

	WHEN("Checking is_polywrap_v") {
		//Compatible_polywrap_value<Parameter_type, Polywrap_type>
		static_assert(prop::detail::is_polywrap_v<prop::Polywrap<int>>);
		static_assert(prop::detail::is_polywrap_v<prop::Polywrap<int> &>);
		static_assert(prop::detail::is_polywrap_v<const volatile prop::Polywrap<int> &&>);
		static_assert(!prop::detail::is_polywrap_v<int>);
		static_assert(!prop::detail::is_polywrap_v<Base>);
		static_assert(!prop::detail::is_polywrap_v<Derived>);
	}

	WHEN("Checking Compatible_polywrap_value") {
		//Compatible_polywrap_value<Parameter_type, Polywrap_type>
		static_assert(prop::detail::Compatible_polywrap_value<int, int>);
		static_assert(prop::detail::Compatible_polywrap_value<int, long>);
		static_assert(prop::detail::Compatible_polywrap_value<long, int>);
		static_assert(prop::detail::Compatible_polywrap_value<Derived, Base>);
		static_assert(!prop::detail::Compatible_polywrap_value<Base, Derived>);
	}

	WHEN("Checking Compatible_polywrap_pointer") {
		struct Pseudo_ptr {
			Base &operator*();
		};

		//Compatible_polywrap_pointer<Parameter_type, Polywrap_type>
		static_assert(prop::detail::Compatible_polywrap_pointer<int *, int>);
		static_assert(prop::detail::Compatible_polywrap_pointer<int *, long>);
		static_assert(prop::detail::Compatible_polywrap_pointer<long *, int>);
		static_assert(prop::detail::Compatible_polywrap_pointer<Derived *, Base>);
		static_assert(!prop::detail::Compatible_polywrap_pointer<Base *, Derived>);
		static_assert(!prop::detail::Compatible_polywrap_pointer<std::unique_ptr<Base>, Derived>);
		static_assert(!prop::detail::Compatible_polywrap_pointer<Pseudo_ptr, Derived>);
		static_assert(prop::detail::Compatible_polywrap_pointer<std::unique_ptr<Derived>, Base>);
		static_assert(prop::detail::Compatible_polywrap_pointer<Pseudo_ptr, Base>);
	}

	WHEN("Checking Compatible_polywrap") {
		//Compatible_polywrap_pointer<Parameter_type, Polywrap_type>
		static_assert(prop::detail::Compatible_polywrap<prop::Polywrap<int>, int>);
		static_assert(!prop::detail::Compatible_polywrap<int, int>);
		static_assert(prop::detail::Compatible_polywrap<prop::Polywrap<Derived>, Base>);
	}

	WHEN("Checking Polywrap_settable") {
		//Polywrap_settable<Parameter_type, Polywrap>
		prop::Polywrap<int> p;
		p.set(42);
		static_assert(prop::detail::Settable<int, prop::Polywrap<int>>);
		static_assert(prop::detail::Settable<int, prop::Polywrap<long>>);
		static_assert(prop::detail::Settable<long, prop::Polywrap<int>>);
		static_assert(prop::detail::Settable<Derived, prop::Polywrap<Base>>);
		static_assert(!prop::detail::Settable<Base, prop::Polywrap<Derived>>);
		static_assert(prop::detail::Settable<int *, prop::Polywrap<int>>);
		static_assert(prop::detail::Settable<int *, prop::Polywrap<long>>);
		static_assert(prop::detail::Settable<long *, prop::Polywrap<int>>);
		static_assert(prop::detail::Settable<Derived *, prop::Polywrap<Base>>);
		static_assert(!prop::detail::Settable<Base *, prop::Polywrap<Derived>>);
	}
}

TEST_CASE("Constructor") {
	static_assert(std::is_same_v<prop::detail::polywrap_type_for_t<std::unique_ptr<int>>, int>);
	static_assert(std::is_same_v<prop::detail::polywrap_type_for_t<int *>, int>);
	static_assert(std::is_same_v<prop::detail::polywrap_type_for_t<prop::Polywrap<int>>, int>);
	static_assert(!prop::is_dereferenceable_v<int>);
	static_assert(std::is_same_v<std::remove_cvref_t<int>, int>);
	static_assert(prop::is_dereferenceable_v<int *>);
	static_assert(prop::is_dereferenceable_v<std::unique_ptr<int>>);
	static_assert(std::is_same_v<decltype(prop::Polywrap{std::make_unique<int>()}), prop::Polywrap<int>>);
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

	struct S {
		int i;
	};
	prop::Polywrap ps = S{};
	ps->i = 42;
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

TEST_CASE("Polymorphism") {
	static bool base_destroyed = false;
	static bool derived_destroyed = false;
	struct Base {
		virtual const char *f() {
			return "Base";
		}
		~Base() { //not virtual on purpose
			base_destroyed = true;
		}
	};
	struct Derived : Base {
		std::unique_ptr<std::function<void()>> message =
			std::make_unique<std::function<void()>>([] { derived_destroyed = true; });
		const char *f() override {
			return "Derived";
		}
		Derived() = default;
		Derived(Derived &&) = default;
		~Derived() {
			if (message) {
				(*message)();
			}
		}
	};
	WHEN("Using unique_ptr") {
		prop::Polywrap<Base> pb;
		pb = std::make_unique<Derived>();
		REQUIRE(*pb->f() == 'D');
		REQUIRE_FALSE(base_destroyed);
		REQUIRE_FALSE(derived_destroyed);
		pb = nullptr;
		REQUIRE(base_destroyed);
		REQUIRE(derived_destroyed);
	}

	WHEN("Using direct values") {
		prop::Polywrap<Base> pb = Derived{};
		REQUIRE(*pb->f() == 'D');
		pb.set(Derived{});
		pb = Derived{};
		REQUIRE(*pb->f() == 'D');
		base_destroyed = derived_destroyed = false;
		pb = nullptr;
		REQUIRE(base_destroyed);
		REQUIRE(derived_destroyed);
	}
}
