#include "../utility/property.h"

#include <catch2/catch.hpp>
#include <memory>

#ifdef PROPERTY_DEBUG
#include "../utility/type_name.h"
#include <iostream>
#define RESET_COUNTER                                                                                                  \
	extern int property_base_counter;                                                                                  \
	property_base_counter = 0;                                                                                         \
	std::clog << '\n' << __LINE__ << '\n';

std::ostream &operator<<(std::ostream &os, const prop::detail::Binding_list &list) {
	const char *separator = "";
	for (const auto &element : list) {
		os << separator << element->name;
		separator = ", ";
	}
	return os;
}

std::ostream &operator<<(std::ostream &os, const prop::detail::Binding_set &list) {
	const char *separator = "";
	for (const auto &element : list) {
		os << separator << element->name;
		separator = ", ";
	}
	return os;
}

template <class T>
concept streamable = requires(std::ostream &os, const T &t) { os << t; };

template <class T>
struct Printer {
	Printer(const T &t)
		: value{t} {}
	const T &value;
	std::ostream &print(std::ostream &os) {
		if constexpr (streamable<T>) {
			return os << value;
		} else {
			return os << prop::type_name<T>() << '@' << &value;
		}
	}
};

template <class T>
std::ostream &operator<<(std::ostream &os, Printer<T> &&printer) {
	return printer.print(os);
}

template <class T>
void prop::print_status(const prop::Property<T> &p) {
	auto &base = *reinterpret_cast<const prop::detail::Property_base *>(static_cast<const void *>(&p));
	std::clog << "Property " << base.name << '\n';
	std::clog << "\tvalue: " << Printer{p.value} << "\n";
	std::clog << "\tsource: " << (p.source ? "Yes" : "No") << "\n";
	std::clog << "\tExplicit dependencies: [" << base.explicit_dependencies << "]\n";
	std::clog << "\tImplicit dependencies: [" << base.implicit_dependencies << "]\n";
	std::clog << "\tDependents: [" << base.dependents << "]\n";
}
#else
#define RESET_COUNTER
#define print_status(PROPERTY)
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

	//compatibility
	struct Gen {
		operator int() const;
	};
	static_assert(!prop::detail::is_property_v<int>);
	static_assert(prop::detail::is_property_v<prop::Property<int>>);
	static_assert(prop::detail::Property_value<Gen, int>);
	static_assert(prop::detail::Compatible_property<prop::Property<Gen>, int>);
	static_assert(prop::detail::Property_function<decltype([] { return Gen{}; }), int>);
	static_assert(!prop::detail::Property_value<prop::Property<int>, int>);
	static_assert(!prop::detail::Property_value<prop::Property<int> &, int>);
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

	print_status(p1);
	print_status(p2);

	int call_counter = 0;
	prop::Property<int> p3;
	p3 = [&] {
		call_counter++;
		if (p1) {
			return p1;
		}
		return p2;
	};

	print_status(p1);
	print_status(p2);
	print_status(p3);

	REQUIRE(p3 == 1);
	REQUIRE(call_counter == 1);
	p1 = 0;
	REQUIRE(p2 == 3);
	REQUIRE(p3 == 3);
	REQUIRE(call_counter == 2);

	print_status(p1);
	print_status(p2);
	print_status(p3);
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

TEST_CASE("Property assignment and value capturing") {
	RESET_COUNTER
	prop::Property p1 = 1;
	prop::Property p2 = p1;
	REQUIRE(p2 == 1);
	p1 = 2;
	REQUIRE(p2 == 1);
	p2 = [&p1] { return p1; };
	REQUIRE(p2 == 2);
	p1 = 1;
	REQUIRE(p2 == 1);
}

TEST_CASE("Moving properties while maintaining binding") {
	RESET_COUNTER
	prop::Property p1 = 1;
	prop::Property<int> p2;
	p2 = {[](int &p1_value) { return p1_value + 1; }, p1};
	REQUIRE(p2 == 2);
	p1 = 2;
	REQUIRE(p2 == 3);

	print_status(p1);
	print_status(p2);

	prop::Property p3 = std::move(p1);

	print_status(p1);
	print_status(p2);
	print_status(p3);

	p3 = 3;

	print_status(p1);
	print_status(p2);
	print_status(p3);

	REQUIRE(p2 == 4);
}

TEST_CASE("Property function binder") {
	prop::Property p = 42;
	prop::detail::Property_function_binder<int>{[] { return 0; }};
	prop::detail::Property_function_binder<int>{[](const prop::Property<int> &p) { return 0; }, p};
	prop::detail::Property_function_binder<int>{[](prop::Property<int> &p) { return 0; }, p};
	prop::detail::Property_function_binder<int>{[](const int &i) { return 0; }, p};
	prop::detail::Property_function_binder<int>{[](int &i) { return 0; }, p};
	prop::detail::Property_function_binder<int>{[](int i) { return 0; }, p};
	prop::detail::Property_function_binder<int>{[](double d) { return 0; }, p};
	prop::detail::Property_function_binder<int>{[](const double &d) { return 0; }, p};
	prop::detail::Property_function_binder<int>{[](int &i) { return 0; }, &p};
	prop::detail::Property_function_binder<int>{[](int i) { return 0; }, &p};
	prop::detail::Property_function_binder<int>{[](double d) { return 0; }, &p};
#if defined PROP_RUN_FAILED_TESTS || 0
	/* Number of function arguments and properties don't match: */
	prop::detail::Property_function_binder<int>{[](int i) { return 0; }};
	/* Callable arguments and parameters are incompatible: */
	prop::detail::Property_function_binder<int>{[](double &d) { return 0; }, p};
#endif
}

TEST_CASE("Explicit bindings") {
	RESET_COUNTER
	prop::Property p1 = 42;
	prop::Property<int> p2;
	p2 = {[](const int &i) { return i + 2; }, p1};
	p2 = {[](int &i) { return i + 2; }, p1};
	p2 = {[](int i) { return i + 2; }, p1};
	p2 = {[](double i) { return i + 2; }, p1};
	p2 = {[](prop::Property<int> &i) { return i + 2; }, p1};
	p2 = {[](prop::Property<int> *i) { return *i + 2; }, p1};
	p2 = {[](const prop::Property<int> &i) { return i + 2; }, p1};
	p2 = {[](const prop::Property<int> *i) { return *i + 2; }, p1};
	p2 = {[](const int &i) { return i + 2; }, &p1};
	p2 = {[](int &i) { return i + 2; }, &p1};
	p2 = {[](int i) { return i + 2; }, &p1};
	p2 = {[](double i) { return i + 2; }, &p1};
	p2 = {[](prop::Property<int> &i) { return i + 2; }, &p1};
	p2 = {[](prop::Property<int> *i) { return *i + 2; }, &p1};
	p2 = {[](const prop::Property<int> &i) { return i + 2; }, &p1};
	p2 = {[](const prop::Property<int> *i) { return *i + 2; }, &p1};
	p2 = {[](int, int, int) { return 42; }, &p1, &p1, p1};
}

TEST_CASE("Expiring explicit bindings") {
	RESET_COUNTER
	prop::Property<int> p1;
	prop::Property p2 = 2;
	{
		prop::Property p3 = 3;
		{
			prop::Property p4 = 4;
			p1 = {[&p2](int &vp3, int *vp4) { return 1 + p2 + vp3 + (vp4 ? *vp4 : 0); }, p3, p4};
			//1 + 2 + 3 + 4 = 10
			REQUIRE(p1 == 10);
			p4 = 44;
			//1 + 2 + 3 + 44 = 50
			REQUIRE(p1 == 50);
		}
		//p4 died, but it's ok because vp4 is a pointer
		//1 + 2 + 3 + 0 = 6
		REQUIRE(p1 == 6);
	}
	//p3 died, so p1 has been severed, old value 6 still exists
	REQUIRE(p1 == 6);
	p2 = 2;
	//changes to p2 have no effect on p1 because p1 has been severed
	REQUIRE(p1 == 6);
}
