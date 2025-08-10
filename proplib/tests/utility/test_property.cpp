#include "proplib/utility/polywrap.h"
#include "proplib/utility/property.h"

#include <catch2/catch_all.hpp>
#include <memory>
#include <numeric>

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
	static_assert(prop::detail::Property_update_function<decltype([] { return Gen{}; }), int>);
	static_assert(!prop::detail::Property_value<prop::Property<int>, int>);
	static_assert(!prop::detail::Property_value<prop::Property<int> &, int>);

	static_assert(prop::detail::has_operator_arrow_v<prop::Property<int>>);
	static_assert(prop::detail::has_operator_arrow_v<prop::Polywrap<int>>);
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
	prop::Property<std::unique_ptr<int>> p1;
	REQUIRE(p1 == nullptr);
	p1 = std::make_unique<int>(42);
	REQUIRE(p1 != nullptr);
	REQUIRE(*p1.get() == 42);
}

struct Generator {
	template <class T>
	operator T() const {
		return {};
	}
};

struct Acceptor {
	Acceptor() = default;
	Acceptor(std::same_as<int> auto v);
	Acceptor &operator=(std::same_as<int> auto) {
		return *this;
	}
};

TEST_CASE("Conversion property value") {
	prop::Property p1 = 42;
	p1 = Generator{};
	prop::Property<Acceptor> p2;
	p2 = 42;
}

TEST_CASE("Dereference") {
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
	prop::detail::Property_function_binder<int>{[](const prop::Property<int> &) { return 0; }, p};
	prop::detail::Property_function_binder<int>{[](prop::Property<int> &) { return 0; }, p};
	prop::detail::Property_function_binder<int>{[](const int &) { return 0; }, p};
	prop::detail::Property_function_binder<int>{[](int &) { return 0; }, p};
	prop::detail::Property_function_binder<int>{[](int) { return 0; }, p};
	prop::detail::Property_function_binder<int>{[](double) { return 0; }, p};
	prop::detail::Property_function_binder<int>{[](const double &) { return 0; }, p};
	prop::detail::Property_function_binder<int>{[](int &) { return 0; }, &p};
	prop::detail::Property_function_binder<int>{[](int) { return 0; }, &p};
	prop::detail::Property_function_binder<int>{[](double) { return 0; }, &p};
#if defined PROP_RUN_FAILED_TESTS || 0
	/* Number of function arguments and properties don't match: */
	prop::detail::Property_function_binder<int>{[](int i) { return 0; }};
	/* Callable arguments and parameters are incompatible: */
	prop::detail::Property_function_binder<int>{[](double &d) { return 0; }, p};
#endif
}

TEST_CASE("Property binding mismatch compilation failures") {
	//prop::Property<int> p = {[](int &, prop::Property<double> &) { return 42; }}; //Not enough properties
	//prop::Property<int> p{{[] { return 42; }, p}}; //Too many properties
	//prop::Property<int> p{{[](double &) {}}}; //incorrect value type
	//prop::Property<int> p{[](int &) {}}; //does not return update status
}

TEST_CASE("Explicit bindings") {
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

struct S {
	prop::Property<int> inner;
};

TEST_CASE("Inner property") {
	prop::Property<S> p1;
	prop::Property<void> p3;
	p3 = {[](S &s) { s.inner = 42; }, p1};
	print_status(p1);
	print_status(p3);
	p1 = S{};
	print_status(p1);
	print_status(p3);
	REQUIRE(p1.get().inner == 42);
	p1 = S{};
	print_status(p1);
	print_status(p3);
	REQUIRE(p1.get().inner == 42);
}

struct SS {
	prop::Property<S> s;
};

TEST_CASE("Inner inner property") {
	prop::Property<SS> p1;
	prop::Property<void> p4;
	p4 = {[](SS &ss) {
			  ss.s = [](S &s) {
				  if (s.inner == 42) {
					  return prop::Updater_result::unchanged;
				  } else {
					  s.inner = 42;
					  return prop::Updater_result::changed;
				  }
			  };
		  },
		  p1};
	print_status(p1);
	print_status(p4);
	p1 = SS{};
	print_status(p1);
	print_status(p4);
	REQUIRE(p1.get().s.get().inner == 42);
	p1 = SS{};
	print_status(p1);
	print_status(p4);
	REQUIRE(p1.get().s.get().inner == 42);
}

TEST_CASE("Apply operators") {
	prop::Property p1 = 42;
	//p1.apply()++;
	//REQUIRE(p1 == 43);
}

TEST_CASE("Apply member function calls") {
	prop::Property<std::vector<int>> p1;
	p1.apply()->push_back(42);
	REQUIRE(p1.get().size() == 1);
	REQUIRE(p1.get().front() == 42);
}

TEST_CASE("Dependency checks") {
	prop::Property p1 = 42;
	prop::Property p2 = [&] { return p1; };
	print_status(p1);
	print_status(p2);
	REQUIRE(p1.is_implicit_dependency_of(p2));
	REQUIRE(p2.is_implicit_dependent_of(p1));
	REQUIRE_FALSE(p1.is_explicit_dependency_of(p2));
	REQUIRE_FALSE(p2.is_explicit_dependent_of(p1));
	REQUIRE(p1.is_dependency_of(p2));
	REQUIRE(p2.is_dependent_on(p1));
	p2 = {[](int i) { return i; }, p1};
	print_status(p1);
	print_status(p2);
	REQUIRE(p1.is_explicit_dependency_of(p2));
	REQUIRE(p2.is_explicit_dependent_of(p1));
	REQUIRE_FALSE(p1.is_implicit_dependency_of(p2));
	REQUIRE_FALSE(p2.is_implicit_dependent_of(p1));
	REQUIRE(p1.is_dependency_of(p2));
	REQUIRE(p2.is_dependent_on(p1));
}

TEST_CASE("Vectorsum") {
	prop::Property<std::vector<int>> pv;
	prop::Property ps = [&pv] { return std::accumulate(std::begin(pv.get()), std::end(pv.get()), 0); };
	REQUIRE(ps == 0);
	pv.apply()->push_back(42);
	REQUIRE(ps == 42);
	pv.apply()->push_back(5);
	REQUIRE(ps == 47);
}

TEST_CASE("Operators") {
	prop::Property<int> pi = 42;
	//pi.apply()++;
	pi++;
	REQUIRE(pi == 43);
	pi++;
	REQUIRE(pi == 44);

	struct S {
		void mf() {}
		void cf() const {}
	};
	prop::Property<S> ps;
	bool ps_changed = false;
	prop::Property<bool> pb = {{[&ps_changed](const S &) { return ps_changed = true; }, ps}};
	REQUIRE(ps_changed == true);
	ps_changed = false;
	ps.apply()->mf();
	REQUIRE(ps_changed == true);
	ps_changed = false;
	ps->cf();
	REQUIRE(ps_changed == false);
}

TEST_CASE("Range-based for-loop") {
	prop::Property pv{std::vector<std::size_t>{0, 1, 2, 3, 4, 5}};
	std::size_t i = 0;
	for (auto &v : pv.get()) {
		REQUIRE(v == i);
		REQUIRE(v == pv[i]);
		i++;
	}
}

#include <filesystem>
#include <stacktrace>

struct Track_function {
	private:
	enum class Function_part { before_name, name, after_name };
	static constexpr std::string_view function_part(std::string_view function, Function_part fp) {
		auto name_end_pos = function.find('(');
		if (name_end_pos == function.npos) {
			name_end_pos = function.size() - 1;
		}
		if (fp == Function_part::after_name) {
			function.remove_prefix(name_end_pos);
			return function;
		}

		auto name_start_pos = std::find_if(std::rbegin(function) + static_cast<long int>(function.size()) -
											   static_cast<long int>(name_end_pos),
										   std::rend(function), [](char c) { return c == ':' or c == ' '; }) -
							  std::rbegin(function);
		name_start_pos = static_cast<long int>(function.size()) - name_start_pos;
		if (fp == Function_part::before_name) {
			function.remove_suffix(function.size() - static_cast<std::size_t>(name_start_pos));
			return function;
		}
		function.remove_suffix(function.size() - name_end_pos);
		function.remove_prefix(static_cast<std::size_t>(name_start_pos));
		return function;
	}

	static std::string_view unquoted(std::string_view v) {
		if (v.size() < 2) {
			return v;
		}
		if (v.front() == '"') {
			v.remove_prefix(1);
		}
		if (v.back() == '"') {
			v.remove_suffix(1);
		}
		return v;
	}

	static constexpr std::stacktrace::size_type max_level = 4;
	struct Trace {
		friend std::ostream &operator<<(std::ostream &os, Trace) {
			auto current_stacktrace = std::stacktrace::current();
			for (auto level = decltype(max_level){2}; level <= max_level; level++) {
				auto &stacktrace = current_stacktrace[level];
				if (not stacktrace) {
					break;
				}
				if (stacktrace.source_file() == "") {
					break;
				}
				os << prop::Color::static_text << "Required from " << prop::Color::file
				   << unquoted(std::filesystem::path{stacktrace.source_file()}.filename().string())
				   << prop::Color::static_text << ':' << prop::Color::file << stacktrace.source_line()
				   << prop::Color::reset << '\n';
			}
			return os;
		}
	} static trace;

	public:
	int operator()() {
		std::clog << prop::Color::function_name << __PRETTY_FUNCTION__ << prop::Color::static_text << " @ "
				  << prop::Color::address << this << '\n'
				  << trace;
		return 42;
	}
	Track_function() {
		std::clog << prop::Color::function_name << __PRETTY_FUNCTION__ << prop::Color::static_text << " @ "
				  << prop::Color::address << this << '\n'
				  << trace;
	}
	~Track_function() {
		std::clog << prop::Color::function_name << __PRETTY_FUNCTION__ << prop::Color::static_text << " @ "
				  << prop::Color::address << this << '\n'
				  << trace;
	}
	Track_function(Track_function &&other) {
		std::clog << prop::Color::function_name << __PRETTY_FUNCTION__ << prop::Color::static_text << " @ "
				  << prop::Color::address << this << prop::Color::static_text << " from " << prop::Color::address
				  << &other << '\n'
				  << trace;
	}
	Track_function &operator=(Track_function &&other) {
		std::clog << prop::Color::function_name << __PRETTY_FUNCTION__ << prop::Color::static_text << " @ "
				  << prop::Color::address << this << prop::Color::static_text << " from " << prop::Color::address
				  << &other << '\n'
				  << trace;
		return *this;
	}
	Track_function(const Track_function &) = delete;
	Track_function &operator=(const Track_function &) = delete;
};

TEST_CASE("Value-captured property") {
	prop::Property ptest{Track_function{}};
	prop::Property p{42};
	prop::Property p2{[p] mutable -> int { return p; }};
	prop::print_status(p2);
	REQUIRE(p2 == 42);
	p++; //no effect since we're not actually bound to p
	REQUIRE(p2 == 42);
}

TEST_CASE("Sever via update function") {
	prop::Property<bool> sever = false;
	prop::Property<int> p{[&sever](int &i) {
		if (i == 42) {
			return sever ? prop::Updater_result::sever : prop::Updater_result::unchanged;
		}
		i = 42;
		return sever ? prop::Updater_result::sever : prop::Updater_result::changed;
	}};
	REQUIRE(p == 42);
	REQUIRE(p.is_bound());
	sever = true;
	REQUIRE(not p.is_bound());
}

TEST_CASE("All types of supported explicit parameters") {
	prop::Property<int> pi;
	pi = prop::detail::Property_function_binder<int>{
		[](int &, int &, int, int *, const int &, const int, const int *, prop::Property<int>, prop::Property<int> &,
		   prop::Property<int> *, const prop::Property<int>, const prop::Property<int> &,
		   const prop::Property<int> *) { return prop::Updater_result::unchanged; },
		pi,
		pi,
		pi,
		pi,
		pi,
		pi,
		pi,
		pi,
		pi,
		pi,
		pi,
		pi,
	};
}
