#include "prop/utility/polywrap.h"
#include "prop/utility/type_traits.h"

#include <catch2/catch_all.hpp>
#include <functional>

static bool base_destroyed = false;
static bool derived_destroyed = false;

namespace {
	struct Base {
		virtual const char *f() {
			return "Base";
		}
		~Base() { //not virtual on purpose
			base_destroyed = true;
		}
	};
	static_assert(std::is_copy_constructible_v<Base> && std::is_copy_assignable_v<Base>);

	struct Copyable_Derived final : Base {
		std::function<void()> message = [] { derived_destroyed = true; };
		const char *f() override final {
			return "Copyable_Derived";
		}
		Copyable_Derived() = default;
		Copyable_Derived(Copyable_Derived &&) = default;
		Copyable_Derived(const Copyable_Derived &) = default;
		Copyable_Derived &operator=(Copyable_Derived &&) = default;
		Copyable_Derived &operator=(const Copyable_Derived &) = default;
		~Copyable_Derived() {
			if (message) {
				message();
			}
		}
		Copyable_Derived(int id_)
			: id{id_} {}
		int id = 0;
	};
	static_assert(std::is_copy_constructible_v<Copyable_Derived> && std::is_copy_assignable_v<Copyable_Derived>);

	struct Non_copyable_Derived final : Base {
		std::unique_ptr<std::function<void()>> message =
			std::make_unique<std::function<void()>>([] { derived_destroyed = true; });
		const char *f() override final {
			return "Non_copyable_Derived";
		}
		Non_copyable_Derived() = default;
		Non_copyable_Derived(Non_copyable_Derived &&) = default;
		~Non_copyable_Derived() {
			if (message) {
				(*message)();
			}
		}
	};
	static_assert(!std::is_copy_constructible_v<Non_copyable_Derived> &&
				  !std::is_copy_assignable_v<Non_copyable_Derived>);
} // namespace

TEST_CASE("Concepts") {
	static_assert(std::is_convertible_v<int, int>);
	static_assert(std::is_convertible_v<Non_copyable_Derived, Non_copyable_Derived>);
	static_assert(std::is_convertible_v<Non_copyable_Derived, Base>);
	static_assert(std::is_convertible_v<Non_copyable_Derived, prop::Polywrap<Base>>);

	WHEN("Checking is_polywrap_v") {
		//Compatible_polywrap_value<Parameter_type, Polywrap_type>
		static_assert(prop::detail::is_polywrap_v<prop::Polywrap<int>>);
		static_assert(prop::detail::is_polywrap_v<prop::Polywrap<int> &>);
		static_assert(prop::detail::is_polywrap_v<const volatile prop::Polywrap<int> &&>);
		static_assert(!prop::detail::is_polywrap_v<int>);
		static_assert(!prop::detail::is_polywrap_v<Base>);
		static_assert(!prop::detail::is_polywrap_v<Copyable_Derived>);
	}

	WHEN("Checking Compatible_polywrap_value") {
		//Compatible_polywrap_value<Parameter_type, Polywrap_type>
		static_assert(prop::detail::Compatible_polywrap_value<int, int>);
		static_assert(prop::detail::Compatible_polywrap_value<int, long>);
		static_assert(prop::detail::Compatible_polywrap_value<long, int>);
		static_assert(prop::detail::Compatible_polywrap_value<Copyable_Derived, Base>);
		static_assert(!prop::detail::Compatible_polywrap_value<Base, Copyable_Derived>);
		static_assert(!prop::detail::Compatible_polywrap_value<Non_copyable_Derived &, Non_copyable_Derived>);
		static_assert(prop::detail::Compatible_polywrap_value<Non_copyable_Derived &&, Non_copyable_Derived>);
	}

	WHEN("Checking Compatible_polywrap_pointer") {
		struct Pseudo_ptr {
			Base &operator*();
		};

		//Compatible_polywrap_pointer<Parameter_type, Polywrap_type>
		static_assert(prop::detail::Compatible_polywrap_pointer<int *, int>);
		static_assert(prop::detail::Compatible_polywrap_pointer<Copyable_Derived *, Base>);
		static_assert(!prop::detail::Compatible_polywrap_pointer<Base *, Copyable_Derived>);
		static_assert(!prop::detail::Compatible_polywrap_pointer<std::unique_ptr<Base>, Copyable_Derived>);
		static_assert(!prop::detail::Compatible_polywrap_pointer<Pseudo_ptr, Copyable_Derived>);
		static_assert(prop::detail::Compatible_polywrap_pointer<std::unique_ptr<Copyable_Derived>, Base>);
		static_assert(prop::detail::Compatible_polywrap_pointer<Pseudo_ptr, Base>);
	}

	WHEN("Checking Compatible_polywrap") {
		//Compatible_polywrap_pointer<Parameter_type, Polywrap_type>
		static_assert(prop::detail::Compatible_polywrap<prop::Polywrap<int>, int>);
		static_assert(!prop::detail::Compatible_polywrap<int, int>);
		static_assert(prop::detail::Compatible_polywrap<prop::Polywrap<Copyable_Derived>, Base>);
		static_assert(prop::detail::Compatible_polywrap<prop::Polywrap<Non_copyable_Derived>, Base>);
		static_assert(!prop::detail::Compatible_polywrap<prop::Polywrap<Non_copyable_Derived> &, Non_copyable_Derived>);
		static_assert(prop::detail::Compatible_polywrap<prop::Polywrap<Non_copyable_Derived> &&, Non_copyable_Derived>);
	}

	WHEN("Checking Polywrap_settable") {
		//Polywrap_settable<Parameter_type, Polywrap>
		prop::Polywrap<int> p;
		p.set(42);
		static_assert(prop::detail::Settable<int, prop::Polywrap<int>>);
		static_assert(prop::detail::Settable<int, prop::Polywrap<long>>);
		static_assert(prop::detail::Settable<long, prop::Polywrap<int>>);
		static_assert(prop::detail::Settable<Copyable_Derived, prop::Polywrap<Base>>);
		static_assert(!prop::detail::Settable<Base, prop::Polywrap<Copyable_Derived>>);
		static_assert(prop::detail::Settable<int *, prop::Polywrap<int>>);
		static_assert(prop::detail::Settable<Copyable_Derived *, prop::Polywrap<Base>>);
		static_assert(!prop::detail::Settable<Base *, prop::Polywrap<Copyable_Derived>>);
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
	WHEN("Using unique_ptr") {
		prop::Polywrap<Base> pb;
		pb = std::make_unique<Copyable_Derived>();
		REQUIRE(*pb->f() == 'C');
		REQUIRE_FALSE(base_destroyed);
		REQUIRE_FALSE(derived_destroyed);
		pb = nullptr;
		REQUIRE(base_destroyed);
		REQUIRE(derived_destroyed);
	}

	WHEN("Using direct values") {
		prop::Polywrap<Base> pb = Copyable_Derived{};
		REQUIRE(*pb->f() == 'C');
		pb.set(Copyable_Derived{});
		pb = Copyable_Derived{};
		REQUIRE(*pb->f() == 'C');
		base_destroyed = derived_destroyed = false;
		pb = nullptr;
		REQUIRE(base_destroyed);
		REQUIRE(derived_destroyed);
	}

	WHEN("Trying to copy a copyable derived") {
		prop::Polywrap<Base> p = Copyable_Derived{};
		auto copy = p;
		REQUIRE(*copy->f() == 'C');
		REQUIRE(p.get() != copy.get());
	}

	WHEN("Assigning a derived") {
		prop::Polywrap<Base> p = Copyable_Derived{1};
		REQUIRE(static_cast<Copyable_Derived *>(p.get())->id == 1);
		p = Copyable_Derived{2};
		REQUIRE(static_cast<Copyable_Derived *>(p.get())->id == 2);
	}

	WHEN("Trying to copy a non-copyable derived") {
		prop::Polywrap<Base> p = Non_copyable_Derived{};
		REQUIRE_THROWS_WITH([&] { auto copy = p; }(),
							"Attempted to copy a " + std::string{prop::type_name<prop::Polywrap<Base>>()} +
								" holding a " + std::string{prop::type_name<Non_copyable_Derived>()} +
								" which is not copy-constructible");
	}

	WHEN("Assigning a derived pointer") {
		prop::Polywrap<Base> p;
		Non_copyable_Derived ncd;
		p = &ncd;
	}
}

TEST_CASE("Copying non-copyable objects") {
	static_assert(!std::is_copy_constructible_v<Non_copyable_Derived>);
	static_assert(!std::is_copy_constructible_v<prop::Polywrap<Non_copyable_Derived>>);
}

TEST_CASE("Swap") {
	prop::Polywrap p1 = 1;
	prop::Polywrap p2 = 2;
	std::swap(p1, p2);
	REQUIRE(*p1 == 2);
	REQUIRE(*p2 == 1);
}
