#include <concepts>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string_view>
#include <type_traits>
#include <utility>

static bool replace(std::string &str, const std::string &from, const std::string &to) {
	size_t start_pos = str.find(from);
	if (start_pos == std::string::npos)
		return false;
	str.replace(start_pos, from.length(), to);
	return true;
}

template <typename T>
std::string type_name() {
	std::string_view name, prefix, suffix;
#ifdef __clang__
	name = __PRETTY_FUNCTION__;
	prefix = "std::string type_name() [T = ";
	suffix = "]";
#elif defined(__GNUC__)
	name = __PRETTY_FUNCTION__;
	prefix = "std::string type_name() [with T = ";
	suffix = "]";
#elif defined(_MSC_VER)
	name = __FUNCSIG__;
	prefix = "std::string __cdecl type_name<";
	suffix = ">(void)";
#endif
	name.remove_prefix(prefix.size());
	name.remove_suffix(suffix.size());
	std::string result = std::string{name};
	replace(result, "std::basic_string<char>", "std::string");
	return result;
}

static std::string center(std::string s, std::size_t width) {
	width -= s.size();
	return std::string(width / 2, ' ') + s + std::string((width + 1) / 2, ' ');
}

template <class From, class To>
constexpr bool converts = requires(From f) { static_cast<To>(f); };

template <bool v, bool vr, bool vrr, bool cv, bool cvr, bool cvrr, class, bool rro = false>
struct Value_category {};
template <class T>
struct Value_category<true, false, true, true, true, true, T> {
	using value_t = T;
};
template <class T>
struct Value_category<true, false, false, true, true, true, T> {
	using value_t = const T;
};
template <class T>
struct Value_category<true, true, false, true, true, false, T> {
	using value_t = T &;
};
template <class T>
struct Value_category<true, false, false, true, true, false, T> {
	using value_t = const T &;
};
template <class T>
struct Value_category<true, false, true, true, true, true, T, true> {
	using value_t = T &&;
};
template <class T>
struct Value_category<false, false, false, false, true, true, T> {
	using value_t = const T &&;
};

template <class Gen, class To>
constexpr bool has_cop_v = requires { &Gen::operator To; };

template <class Gen, class To>
using value_category_t =
	Value_category<converts<Gen, To>, converts<Gen, To &>, converts<Gen, To &&>, converts<Gen, const To>,
				   converts<Gen, const To &>, converts<Gen, const To &&>, To, has_cop_v<Gen, To &&>>::value_t;

template <class From, class To>
std::string success_string(std::size_t column_size) {
	return " | " + center({converts<From, To>()}, column_size - 3);
}

template <class... Args>
struct Type_list {};

static std::map<std::size_t, std::size_t> name_sizes;

template <class From, class... To, std::size_t... indexes>
std::string make_conversion_line(std::index_sequence<indexes...>) {
	return (... + success_string<From, To>(name_sizes[indexes]));
}

template <class... From, class... To, std::size_t... indexes>
void make_conversion_table(Type_list<From...>, Type_list<To...>, std::index_sequence<indexes...>) {
	std::array<std::size_t, sizeof...(From)> local_name_sizes = {(type_name<From>().size() + 3)...};
	for (std::size_t i = 0; i < sizeof...(From); i++) {
		name_sizes[i] = std::max(name_sizes[i], local_name_sizes[i]);
	}
	std::array<std::size_t, sizeof...(From)> name_pos = {};
	std::size_t max_name_size = std::max_element(std::begin(name_sizes), std::end(name_sizes),
												 [](auto lhs, auto rhs) { return lhs.second < rhs.second; })
									->second -
								3;
	const std::string table_head = "Converts To\\From";
	max_name_size = std::max(max_name_size, table_head.size());
	for (std::size_t pos = 1; pos < std::size(name_pos); pos++) {
		name_pos[pos] = name_pos[pos - 1] + name_sizes[pos - 1];
	}
	std::cout << table_head << std::string(max_name_size - table_head.size(), ' ');
	(..., (std::cout << "   " << center(type_name<From>(), name_sizes[indexes] - 3)));
	std::cout << '\n';
	std::string line = (... + (" | " + std::string(type_name<From>().size(), ' ')));
	std::string from_types[] = {
		(std::string{type_name<From>()} + std::string(max_name_size - type_name<From>().size(), ' '))...};
	std::string lines[sizeof...(From)] = {{make_conversion_line<From, To...>(std::index_sequence_for<To...>())}...};
	for (std::size_t y = 0; y < sizeof...(From); y++) {
		std::cout << from_types[y];
		std::cout << lines[y];
		std::cout << '\n';
	}
}

template <class... Args>
void make_conversion_table() {
	make_conversion_table(Type_list<Args...>{}, Type_list<Args...>{}, std::index_sequence_for<Args...>());
}

template <class T>
void make_conversion_table_from() {
	make_conversion_table<T, T &&, T &, const T, const T &, const T &&>();
}

struct S {
	operator int *() const {
		return nullptr;
	}
	int i;
	operator const int &() const {
		return i;
	}
	operator int &() {
		return i;
	}
	template <class T>
		requires std::same_as<T, double>
	operator T() const {
		return 3.14;
	}
	operator std::unique_ptr<int>() {
		return nullptr;
	}
	operator std::mutex() {
		return {};
	}
};

template <class T>
struct Wrapper {
	T t;
#if 1
	template <class U>
		requires(std::is_same_v<U, value_category_t<T, U>>)
	operator U() {
		return t;
	}
	template <class U>
		requires(std::is_same_v<U &, value_category_t<T, U>>)
	operator U &() {
		return t;
	}
	template <class U>
		requires(std::is_same_v<U &&, value_category_t<T, U>>)
	operator U &&() {
		return t;
	}
	template <class U>
		requires(std::is_same_v<const U, value_category_t<T, U>>)
	operator const U() {
		return t;
	}
	template <class U>
		requires(std::is_same_v<const U &, value_category_t<T, U>>)
	operator const U &() {
		return t;
	}
	template <class U>
		requires(std::is_same_v<const U &&, value_category_t<T, U>>)
	operator const U &&() {
		return t;
	}
	template <class U>
		requires(std::is_same_v<void, value_category_t<T, U>>)
	operator U() {
		return t;
	}
	template <class U>
		requires(std::is_same_v<U, value_category_t<const T, U>>)
	operator U() const {
		return t;
	}
	template <class U>
		requires(std::is_same_v<U &, value_category_t<const T, U>>)
	operator U &() const {
		return t;
	}
	template <class U>
		requires(std::is_same_v<U &&, value_category_t<const T, U>>)
	operator U &&() const {
		return t;
	}
	template <class U>
		requires(std::is_same_v<const U, value_category_t<const T, U>>)
	operator const U() const {
		return t;
	}
	template <class U>
		requires(std::is_same_v<const U &, value_category_t<const T, U>>)
	operator const U &() const {
		return t;
	}
	template <class U>
		requires(std::is_same_v<const U &&, value_category_t<const T, U>>)
	operator const U &&() const {
		return t;
	}
	template <class U>
		requires(std::is_same_v<void, value_category_t<const T, U>>)
	operator U() const {
		return t;
	}
#endif
};

template <auto F>
void test1() {
	S s;
	if constexpr (requires { F(s); }) { //if F(S) compiles
		Wrapper<S> w;
		F(w); //then F(Wrapper<S>) must also compile
	}
}

template <class... Ts>
void test1(Type_list<Ts...>) {
	(..., test1<+[](Ts) {}>());
}

template <class T>
void test2() {
	S s;
	if constexpr (requires { static_cast<T>(s); }) { //if static_cast<T>(S) compiles
		Wrapper<S> w;
		(void)static_cast<T>(w); //then static_cast<T>(Wrapper<S>) must also compile
	}
	if constexpr (requires { T{s}; }) { //if static_cast<T>(S) compiles
		(void)Wrapper<S>{s};
	}
	if constexpr (std::is_constructible_v<T, S &>) {			 //if T = S; compiles
		static_assert(std::is_constructible_v<T, Wrapper<S> &>); //then T = Wrapper<S>; must also compile
	}
	if constexpr (std::is_constructible_v<T, const S &>) {			   //if T = const S; compiles
		static_assert(std::is_constructible_v<T, const Wrapper<S> &>); //then T = const Wrapper<S>; must also compile
	}
}

template <class... Ts>
void test2(Type_list<Ts...>) {
	(..., test2<Ts>());
}

int main() {
	//make_conversion_table_from<std::string>();
	//make_conversion_table_from<int>();
	static Type_list<void *, int, int &, int &&, const int, const int &, const int &&, int *, int &, double &&,
					 std::unique_ptr<int>, std::mutex>
		types;
	test1(types);
	test2(types);
}
