#include "../utility/property.h"

#include <catch2/catch.hpp>
#include <memory>

#ifdef PROPERTY_DEBUG
#include <iostream>
#define RESET_COUNTER                                                                                                  \
	extern int property_base_counter;                                                                                  \
	property_base_counter = 0;                                                                                         \
	std::clog << __LINE__ << '\n';
#else
#define RESET_COUNTER ;
#endif

TEST_CASE("Compile time checks") {
	//Primitive types
	static_assert(std::is_same_v<prop::detail::inner_value_type_t<int>, int>);
	static_assert(std::is_same_v<prop::detail::inner_value_type_t<const volatile int &>, int>);
	static_assert(std::is_same_v<prop::detail::inner_type_t<int>, int>);
	static_assert(std::is_same_v<prop::detail::inner_type_t<const volatile int &>, int>);

	//Property types
	static_assert(std::is_same_v<prop::detail::inner_value_type_t<prop::Property<int>>, int>);
	static_assert(std::is_same_v<prop::detail::inner_value_type_t<const prop::Property<int>>, int>);
	static_assert(std::is_same_v<prop::detail::inner_value_type_t<prop::Property<int> &>, int>);
	static_assert(std::is_same_v<prop::detail::inner_value_type_t<prop::Property<int> &&>, int>);
	static_assert(std::is_same_v<prop::detail::inner_value_type_t<volatile prop::Property<int> &>, int>);
	static_assert(std::is_same_v<prop::detail::inner_type_t<prop::Property<int>>, int>);

	//functional types
	static_assert(std::is_same_v<prop::detail::inner_function_type_t<std::function<int()>>, int>);
	static_assert(std::is_same_v<prop::detail::inner_function_type_t<std::function<const volatile int && ()>>, int>);
	static_assert(std::is_same_v<prop::detail::inner_function_type_t<int (*)()>, int>);
	static_assert(std::is_same_v<prop::detail::inner_function_type_t<decltype([] { return 42; })>, int>);

	static_assert(std::is_same_v<prop::detail::inner_type_t<std::function<int()>>, int>);
	static_assert(std::is_same_v<prop::detail::inner_type_t<std::function<const volatile int && ()>>, int>);
	static_assert(std::is_same_v<prop::detail::inner_type_t<int (*)()>, int>);
	static_assert(std::is_same_v<prop::detail::inner_type_t<decltype([] { return 42; })>, int>);

	//functions returning properties
	static_assert(std::is_same_v<prop::detail::inner_function_type_t<std::function<prop::Property<int>()>>, int>);
	static_assert(
		std::is_same_v<prop::detail::inner_function_type_t<std::function<const volatile prop::Property<int> && ()>>,
					   int>);
	static_assert(std::is_same_v<prop::detail::inner_function_type_t<prop::Property<int> (*)()>, int>);
	static_assert(
		std::is_same_v<prop::detail::inner_function_type_t<decltype([] { return prop::Property<int>(42); })>, int>);

	static_assert(std::is_same_v<prop::detail::inner_type_t<std::function<prop::Property<int>()>>, int>);
	static_assert(
		std::is_same_v<prop::detail::inner_type_t<std::function<const volatile prop::Property<int> && ()>>, int>);
	static_assert(std::is_same_v<prop::detail::inner_type_t<prop::Property<int> (*)()>, int>);
	static_assert(std::is_same_v<prop::detail::inner_type_t<decltype([] { return prop::Property<int>(42); })>, int>);
}

TEST_CASE("Basic Property tests") {
	prop::Property p1 = 42;
	REQUIRE(p1 == 42);
	p1 = [] { return 12; };
	REQUIRE(p1 == 12);
	prop::Property<int> p2;
	p2 = [&p1] { return p1 + 3; };
	REQUIRE(p2 == 15);
	p1 = 1;
	REQUIRE(p2 == 4);

	int call_counter = 0;
	prop::Property<int> p3;
	p3 = [&] {
		call_counter++;
		if (p1) {
			return p1;
		}
		return p2;
	};
	REQUIRE(p3 == 1);
	REQUIRE(call_counter == 1);
	p1 = 0;
	REQUIRE(p2 == 3);
	REQUIRE(p3 == 3);
	REQUIRE(call_counter == 2);
}

TEST_CASE("Property<Move_only>") {
	RESET_COUNTER
	prop::Property<std::unique_ptr<int>> p1;
	REQUIRE(p1 == nullptr);
	p1 = std::make_unique<int>(42);
	REQUIRE(p1 != nullptr);
	REQUIRE(*p1.get() == 42);
}

TEST_CASE("Dereference") {
	RESET_COUNTER
	struct S {
		int i = 42;
	};
	WHEN("Using a class") {
		S s;
		prop::Property p1{s};
		REQUIRE(p1.get().i == 42);
	}
	WHEN("Using a class pointer") {
		S s;
		prop::Property p2 = &s;
		REQUIRE(p2.get()->i == 42);
	}

	WHEN("Using unique_ptr") {
		prop::Property p3 = std::make_unique<S>();
		REQUIRE(p3.get()->i == 42);
	}
}

TEST_CASE("Property assignment and binding") {
	RESET_COUNTER
	prop::Property p1 = 1;
	prop::Property p2 = p1;
	REQUIRE(p2 == 1);
	p1 = 2;
	REQUIRE(p2 == 1);
	p2.bind(p1);
	REQUIRE(p2 == 2);
	p1 = 1;
	REQUIRE(p2 == 1);
}

TEST_CASE("Moving properties while maintaining binding") {
	RESET_COUNTER
	prop::Property p1 = 1;
	prop::Property<int> p2;
	p2.bind(p1);
	REQUIRE(p2 == 1);
	p1 = 2;
	REQUIRE(p2 == 2);
	prop::Property p3 = std::move(p1);
	p3 = 3;
	REQUIRE(p2 == 3);
}
