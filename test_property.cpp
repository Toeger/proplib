#include "property.h"
#include <catch2/catch.hpp>

TEST_CASE("Compile time checks") {
	//Primitive types
	static_assert(std::is_same_v<pro::detail::inner_value_type_t<int>, int>);
	static_assert(std::is_same_v<pro::detail::inner_value_type_t<const volatile int &>, int>);
	static_assert(std::is_same_v<pro::detail::inner_type_t<int>, int>);
	static_assert(std::is_same_v<pro::detail::inner_type_t<const volatile int &>, int>);

	//Property types
	static_assert(std::is_same_v<pro::detail::inner_value_type_t<pro::Property<int>>, int>);
	static_assert(std::is_same_v<pro::detail::inner_value_type_t<const pro::Property<int>>, int>);
	static_assert(std::is_same_v<pro::detail::inner_value_type_t<pro::Property<int> &>, int>);
	static_assert(std::is_same_v<pro::detail::inner_value_type_t<pro::Property<int> &&>, int>);
	static_assert(std::is_same_v<pro::detail::inner_value_type_t<volatile pro::Property<int> &>, int>);
	static_assert(std::is_same_v<pro::detail::inner_type_t<pro::Property<int>>, int>);

	//functional types
	static_assert(std::is_same_v<pro::detail::inner_function_type_t<std::function<int()>>, int>);
	static_assert(std::is_same_v<pro::detail::inner_function_type_t<std::function<const volatile int && ()>>, int>);
	static_assert(std::is_same_v<pro::detail::inner_function_type_t<int (*)()>, int>);
	static_assert(std::is_same_v<pro::detail::inner_function_type_t<decltype([] { return 42; })>, int>);

	static_assert(std::is_same_v<pro::detail::inner_type_t<std::function<int()>>, int>);
	static_assert(std::is_same_v<pro::detail::inner_type_t<std::function<const volatile int && ()>>, int>);
	static_assert(std::is_same_v<pro::detail::inner_type_t<int (*)()>, int>);
	static_assert(std::is_same_v<pro::detail::inner_type_t<decltype([] { return 42; })>, int>);

	//functions returning properties
	static_assert(std::is_same_v<pro::detail::inner_function_type_t<std::function<pro::Property<int>()>>, int>);
	static_assert(
		std::is_same_v<pro::detail::inner_function_type_t<std::function<const volatile pro::Property<int> && ()>>,
					   int>);
	static_assert(std::is_same_v<pro::detail::inner_function_type_t<pro::Property<int> (*)()>, int>);
	static_assert(
		std::is_same_v<pro::detail::inner_function_type_t<decltype([] { return pro::Property<int>(42); })>, int>);

	static_assert(std::is_same_v<pro::detail::inner_type_t<std::function<pro::Property<int>()>>, int>);
	static_assert(
		std::is_same_v<pro::detail::inner_type_t<std::function<const volatile pro::Property<int> && ()>>, int>);
	static_assert(std::is_same_v<pro::detail::inner_type_t<pro::Property<int> (*)()>, int>);
	static_assert(std::is_same_v<pro::detail::inner_type_t<decltype([] { return pro::Property<int>(42); })>, int>);
}

TEST_CASE("Basic Property tests") {
	pro::Property p = 42;
	REQUIRE(p == 42);
	p = [] { return 12; };
	REQUIRE(p == 12);
	pro::Property<int> p2;
	p2 = [&p] { return p + 3; };
	REQUIRE(p2 == 15);
	p = 1;
	REQUIRE(p2 == 4);

	int call_counter = 0;
	pro::Property<int> p3;
	p3 = [&] {
		call_counter++;
		if (p) {
			return +p;
		}
		return +p2;
	};
	REQUIRE(p3 == 1);
	REQUIRE(call_counter == 1);
	p = 0;
	REQUIRE(p2 == 3);
	REQUIRE(p3 == 3);
	REQUIRE(call_counter == 2);
}
