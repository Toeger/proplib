#pragma once

#include "callable.h"
#include "color.h"
#include "exceptions.h"
#include "raii.h"
#include "type_list.h"
#include "type_name.h"
#include "type_traits.h"

#include <cassert>
#include <concepts>
#include <functional>
#include <iostream>
#include <set>
#include <vector>

#ifdef PROPERTY_NAMES
#include <string>
#endif

namespace prop {
	template <class T>
	class Property;

	enum struct Value : bool { unchanged, changed };

	class Dependency_tracer;

	namespace detail {
		template <class T, class U = T>
		concept is_equal_comparable_v = requires(const T &t, const U &u) {
			{ t == u } -> std::convertible_to<bool>;
		};

		template <class T, class U>
		constexpr bool is_equal(const T &lhs, const U &rhs) {
			if constexpr (is_equal_comparable_v<T, U>) {
				return lhs == rhs;
			}
			// TODO: Make an attempt to compare containers
			return false;
		}

#ifdef PROPERTY_NAMES
		std::string to_string(const void *p);
#endif

		struct Property_base;

		struct Binding_set {
			bool has(const Property_base *) const;
			bool is_empty() const;
			bool add(Property_base *);
			void remove(Property_base *);
			void clear();
			void replace(Property_base *old_value, Property_base *new_value);
			std::set<Property_base *>::iterator begin();
			std::set<Property_base *>::iterator end();
			std::set<Property_base *>::const_iterator begin() const;
			std::set<Property_base *>::const_iterator end() const;
			std::set<Property_base *>::const_iterator cbegin() const;
			std::set<Property_base *>::const_iterator cend() const;
			std::set<Property_base *> set; //TODO: Try out flat_set or something to save some nanoseconds
		};
		std::ostream &operator<<(std::ostream &os, const Binding_set &set);

		struct Binding_list {
			bool has(const Property_base *property) const;
			std::size_t size() const;
			void replace(Property_base *old_value, Property_base *new_value);
			Property_base *operator[](std::size_t index) const;
			std::vector<Property_base *>::iterator begin();
			std::vector<Property_base *>::iterator end();
			std::vector<Property_base *>::const_iterator begin() const;
			std::vector<Property_base *>::const_iterator end() const;
			std::vector<Property_base *> list; //TODO: Try out other things like std::forward_list
		};
		std::ostream &operator<<(std::ostream &os, const prop::detail::Binding_list &list);

		void swap(Property_base &lhs, Property_base &rhs);

		struct Property_base {
			virtual void update();
			virtual void unbind();
			void unbind_depends();
			void read_notify() const;
			void read_notify();
			void write_notify();
			void update_start();
			void update_complete();
			void set_explicit_dependencies(Binding_list list);
			bool is_implicit_dependency_of(const Property_base &other) const;
			bool is_explicit_dependency_of(const Property_base &other) const;
			bool is_dependency_of(const Property_base &other) const;
			bool is_implicit_dependent_of(const Property_base &other) const;
			bool is_explicit_dependent_of(const Property_base &other) const;
			bool is_dependent_on(const Property_base &other) const;

#ifdef PROPERTY_NAMES
			Property_base(std::string_view type_name);
#else
			Property_base();
#endif
			Property_base(const Property_base &) = delete;
			Property_base(Property_base &&other);
			void operator=(const Property_base &) = delete;
			void operator=(Property_base &&other);

			bool need_update = false;
			static inline Property_base *current_binding;
			Property_base *previous_binding = nullptr;
			Property_base *creation_binding = current_binding;
			void clear_implicit_dependencies();
			void sever_implicit_dependents();
			void take_explicit_dependents(Property_base &&source);
			friend void swap(Property_base &lhs, Property_base &rhs);
			Binding_list explicit_dependencies;
			const Binding_set &get_implicit_dependencies() const;
			const Binding_set &get_dependents() const;
#ifdef PROPERTY_NAMES
			std::string_view type;
			std::string custom_name;
			std::string get_name() const {
				std::string auto_name = prop::to_string(prop::Console_text_color{prop::Color::type}) +
										std::string{type} +
										prop::to_string(prop::Console_text_color{prop::Color::static_text}) + "@" +
										prop::to_string(prop::Console_text_color{prop::Color::address}) +
										prop::detail::to_string(this) + prop::to_string(prop::console_reset_text_color);
				if (custom_name.empty()) {
					return auto_name;
				}
				return auto_name + ' ' + custom_name;
			}
#endif

			protected:
			~Property_base();

			private:
			Binding_set implicit_dependencies;
			Binding_set dependents;
			friend Dependency_tracer;
		};

		template <class T>
		struct Property_name_base : Property_base {
#ifdef PROPERTY_NAMES
			Property_name_base()
				: prop::detail::Property_base("") {
				type = prop::type_name<T>();
			}

			protected:
			~Property_name_base() = default;
#endif
		};

		template <class Property>
		constexpr bool is_property_v = is_type_specialization_v<std::remove_cvref_t<Property>, prop::Property>;

		template <class Compatible_type, class Inner_property_type>
		concept Property_value =
			std::convertible_to<Compatible_type, Inner_property_type> && !is_property_v<Compatible_type>;

		template <class Property, class Inner_property_type>
		concept Compatible_property =
			is_property_v<Property> && (!std::is_lvalue_reference_v<Property> || requires(Property &&p) {
				{ p.get() } -> Property_value<Inner_property_type>;
			});

		namespace {
			template <class Function, class T, class... Properties>
			concept Property_value_function = requires(Function &&f) {
				{ f(std::declval<Properties>()...) } -> Property_value<T>;
			};

			template <class Function, class T, class... Properties>
			concept Compatible_property_function = requires(Function &&f) {
				{ f(std::declval<Properties>()...) } -> Compatible_property<T>;
			};

			template <class Function, class T, class... Properties>
			concept Value_result_function = requires(Function &&f) {
				{ f(std::declval<T &>(), std::declval<Properties>()...) } -> std::same_as<prop::Value>;
			};
		} // namespace

		template <class Function, class T, class... Properties>
		concept Property_update_function = Property_value_function<Function, T, Properties...> ||
										   Compatible_property_function<Function, T, Properties...> ||
										   Value_result_function<Function, T, Properties...>;

		template <class T>
		Property_base *get_property_base_pointer(const prop::Property<T> *p) {
			return static_cast<Property_base *>(const_cast<prop::Property<T> *>(p));
		}
		template <class T>
		Property_base *get_property_base_pointer(const prop::Property<T> &p) {
			return get_property_base_pointer(&p);
		}

		template <class Function_arg, class Property>
		consteval bool is_compatible_function_arg_for_property() {
			using Prop_t = std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<Property>>>;
			using T = std::remove_cvref_t<decltype(std::declval<Prop_t>().get())>;
			if (std::is_constructible_v<Function_arg, Prop_t &>) {
				return true;
			} else if (std::is_constructible_v<Function_arg, Prop_t *>) {
				return true;
			} else if (std::is_constructible_v<Function_arg, T &>) {
				return true;
			} else if (std::is_constructible_v<Function_arg, T *>) {
				return true;
			}
			return false;
		}

		template <class Function_args_list, class Properties_list, std::size_t... indexes>
		consteval bool are_compatible_function_args_for_properties(std::index_sequence<indexes...>) {
			return (is_compatible_function_arg_for_property<typename Function_args_list::template at<indexes>,
															typename Properties_list::template at<indexes>>() &&
					...);
		}

		template <class Function_arg, class Property>
		consteval bool requires_valid_property() {
			using Prop_t = std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<Property>>>;
			using T = std::remove_cvref_t<decltype(std::declval<Prop_t>().get())>;
			if (std::is_constructible_v<Function_arg, Prop_t &>) {
				return true;
			} else if (std::is_constructible_v<Function_arg, Prop_t *>) {
				return false;
			} else if (std::is_constructible_v<Function_arg, T &>) {
				return true;
			} else if (std::is_constructible_v<Function_arg, T *>) {
				return false;
			}
		}

		template <class Function_arg, class Property>
		consteval bool changes_property() {
			using Prop_t = std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<Property>>>;
			using T = std::remove_cvref_t<decltype(std::declval<Prop_t>().get())>;
			if constexpr (std::is_constructible_v<Function_arg, const Prop_t &>) {
				return false;
			} else if constexpr (std::is_constructible_v<Function_arg, const Prop_t *>) {
				return false;
			} else if constexpr (std::is_constructible_v<Function_arg, Prop_t &>) {
				return true;
			} else if constexpr (std::is_constructible_v<Function_arg, Prop_t *>) {
				return true;
			} else if constexpr (std::is_constructible_v<Function_arg, const T &>) {
				return false;
			} else if constexpr (std::is_constructible_v<Function_arg, const T *>) {
				return false;
			} else if constexpr (std::is_constructible_v<Function_arg, T &>) {
				return true;
			} else if constexpr (std::is_constructible_v<Function_arg, T *>) {
				return true;
			}
		}

		template <class Function_arg, class Property>
		decltype(auto) convert_to_function_arg(const Property *property) {
			using T = std::remove_cvref_t<decltype(property->get())>;
			if constexpr (std::is_constructible_v<Function_arg, const Property &>) {
				if (!property) {
					throw prop::Property_expired{""};
				}
				return *property;
			} else if constexpr (std::is_constructible_v<Function_arg, const Property *>) {
				return property;
			} else if constexpr (std::is_constructible_v<Function_arg, Property &>) {
				if (!property) {
					throw prop::Property_expired{""};
				}
				return *const_cast<Property *>(property);
			} else if constexpr (std::is_constructible_v<Function_arg, Property *>) {
				return const_cast<Property *>(property);
			} else if constexpr (std::is_constructible_v<Function_arg, const T &>) {
				if (!property) {
					throw prop::Property_expired{""};
				}
				return property->get();
			} else if constexpr (std::is_constructible_v<Function_arg, const T *>) {
				return property ? &property->get() : nullptr;
			} else if constexpr (std::is_constructible_v<Function_arg, T &>) {
				if (!property) {
					throw prop::Property_expired{""};
				}
				return const_cast<T &>(property->get());
			} else if constexpr (std::is_constructible_v<Function_arg, T *>) {
				return property ? &const_cast<T &>(property->get()) : nullptr;
			}
		}

		template <class T, class Function, class... Properties, std::size_t... indexes>
			requires(not std::is_same_v<T, void>)
		std::move_only_function<prop::Value(T &, const Binding_list &)>
		create_explicit_caller(Function &&function, std::index_sequence<indexes...>) {
#define PROP_ARGS                                                                                                      \
	convert_to_function_arg<typename Args_list::template at<indexes>>(                                                 \
		static_cast<std::remove_pointer_t<std::remove_cvref_t<Properties>> *>(explicit_dependencies[indexes]))
			return [source = std::forward<Function>(function)](
					   T &t, const Binding_list &explicit_dependencies) mutable -> prop::Value {
				using Args_list = prop::Callable_info_for<Function>::Args;
				if constexpr (Args_list::size == sizeof...(indexes)) {
					if constexpr (prop::detail::is_equal_comparable_v<T, decltype(source(PROP_ARGS...))>) {
						auto value = source(PROP_ARGS...);
						if (prop::detail::is_equal(t, value)) {
							return prop::Value::unchanged;
						} else {
							t = std::move(value);
							return prop::Value::changed;
						}
					} else {
						t = source(PROP_ARGS...);
						return prop::Value::changed;
					}
				} else if constexpr (Args_list::size == sizeof...(indexes) + 1) {
					static_assert(std::is_invocable_v<decltype(source), decltype(t), decltype(PROP_ARGS)...>,
								  "Updating function parameters do not match property type(s)");
					static_assert(
						std::is_same_v<std::invoke_result_t<decltype(source), decltype(t), decltype(PROP_ARGS)...>,
									   prop::Value>,
						"Updating function taking value reference must return prop::Value::changed or "
						"prop::Value::unchanged");

					return source(t, PROP_ARGS...);
				} else {
					static_assert(false, "Number of parameters does not match number of given properties");
				}
			};
#undef PROP_ARGS
		}

		template <class T, class Function, class... Properties, std::size_t... indexes>
			requires(std::is_same_v<T, void>)
		std::move_only_function<void(const Binding_list &)> create_explicit_caller(Function &&function,
																				   std::index_sequence<indexes...>) {
			return [source = std::forward<Function>(function)](const Binding_list &explicit_dependencies) mutable {
				using Args_list = prop::Callable_info_for<Function>::Args;
				if constexpr (Args_list::size == sizeof...(indexes)) {
					source(convert_to_function_arg<typename Args_list::template at<indexes>>(
						static_cast<std::remove_pointer_t<std::remove_cvref_t<Properties>> *>(
							explicit_dependencies[indexes]))...);
				} else {
					static_assert(false, "Number of parameters does not match number of given properties");
				}
			};
		}

#define PROP_ACTUALLY_PROPERTIES                                                                                       \
	prop::is_type_specialization_v<std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<Properties>>>,        \
								   prop::Property> and...
		//TODO: Compatible function test
		template <class T>
		struct Property_function_binder {
			template <class... Properties, class Function>
				requires(PROP_ACTUALLY_PROPERTIES)
			Property_function_binder(Function &&function_, Properties &&...properties)
				: function{create_explicit_caller<T, decltype(function_), Properties...>(
					  std::forward<decltype(function_)>(function_), std::index_sequence_for<Properties...>{})}
				, explicit_dependencies{{prop::detail::get_property_base_pointer(properties)...}} {}

			template <class... Properties, class Function>
				requires(sizeof...(Properties) > 0 and not(PROP_ACTUALLY_PROPERTIES))
			Property_function_binder(Function &&, Properties &&...) {
				static_assert((PROP_ACTUALLY_PROPERTIES), "Passed argument is not a prop::Property");
			}

			std::move_only_function<prop::Value(T &, const Binding_list &)> function;
			Binding_list explicit_dependencies;
		};

		template <>
		struct Property_function_binder<void> {
			template <class Function, class... Properties>
			Property_function_binder(Function &&function_, Properties &&...properties)
				requires(PROP_ACTUALLY_PROPERTIES)
				: function{create_explicit_caller<void, Function, Properties...>(
					  std::forward<Function>(function_), std::index_sequence_for<Properties...>{})}
				, explicit_dependencies{{prop::detail::get_property_base_pointer(properties)...}} {
				static_assert(prop::Callable_info_for<Function>::Args::size == sizeof...(Properties),
							  "Number of function arguments and properties don't match");
				static_assert(
					are_compatible_function_args_for_properties<typename prop::Callable_info_for<Function>::Args,
																prop::Type_list<Properties...>>(
						std::index_sequence_for<Properties...>{}),
					"Callable arguments and parameters are incompatible");
			}

			template <class Function, class... Properties>
				requires(not PROP_ACTUALLY_PROPERTIES)
			Property_function_binder(Function &&, Properties &&...) {
				static_assert((PROP_ACTUALLY_PROPERTIES),
							  "Arguments to update function must be specializations of prop::Property");
			}

			std::move_only_function<void(const Binding_list &)> function;
			Binding_list explicit_dependencies;
		};
#undef PROP_ACTUALLY_PROPERTIES

		template <class T>
		std::move_only_function<prop::Value(T &, const prop::detail::Binding_list &)>
		make_direct_update_function(Property_value_function<T> auto &&f) {
			return [source = std::forward<decltype(f)>(f)](T &t, const prop::detail::Binding_list &) mutable {
				auto value = source();
				if (is_equal(value, t)) {
					return prop::Value::unchanged;
				}
				t = std::move(value);
				return prop::Value::changed;
			};
		}

		template <class T>
		std::move_only_function<prop::Value(T &, const prop::detail::Binding_list &)>
		make_direct_update_function(Compatible_property_function<T> auto &&f) {
			return [source = std::forward<decltype(f)>(f)](T &t, const prop::detail::Binding_list &) mutable {
				if constexpr (prop::detail::is_equal_comparable_v<std::decay_t<decltype(source())>, T>) {
					auto value = source();
					if (is_equal(value, t)) {
						return prop::Value::unchanged;
					}
					t = std::move(value);
					return prop::Value::changed;
				} else {
					t = source();
					return prop::Value::changed;
				}
			};
		}

		template <class T>
		std::move_only_function<prop::Value(T &, const prop::detail::Binding_list &)>
		make_direct_update_function(Value_result_function<T> auto &&f) {
			return [source = std::forward<decltype(f)>(f)](T &t, const prop::detail::Binding_list &) mutable {
				if constexpr (prop::detail::is_equal_comparable_v<std::decay_t<decltype(source())>, T>) {
					auto value = source();
					//if (is_equal(value, t)) {
					//	return prop::Value::unchanged;
					//}
					t = std::move(value);
					return prop::Value::changed;
				} else {
					t = source();
					return prop::Value::changed;
				}
			};
		}

		template <class T>
		concept Streamable = requires(std::ostream &os, const T &t) { os << t; };

		template <class T>
		struct Printer {
			Printer(const T &t)
				: value{t} {}
			const T &value;
			std::ostream &print(std::ostream &os) {
				if constexpr (Streamable<T>) {
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

		template <typename T>
		concept has_operator_arrow_v = requires(T &&t) { t.operator->(); } || std::is_pointer_v<T>;

		template <class T, class U>
		constexpr auto &&forward_like(U &&x) noexcept {
			constexpr bool is_adding_const = std::is_const_v<std::remove_reference_t<T>>;
			if constexpr (std::is_lvalue_reference_v<T &&>) {
				if constexpr (is_adding_const)
					return std::as_const(x);
				else
					return static_cast<U &>(x);
			} else {
				if constexpr (is_adding_const)
					return std::move(std::as_const(x));
				else
					return std::move(x);
			}
		}
	} // namespace detail

	extern void (*on_property_severed)(prop::detail::Property_base *severed, prop::detail::Property_base *reason);
	extern void (*on_property_update_exception)(std::exception_ptr exception);

	template <class U>
	void print_status(const prop::Property<U> &p, std::ostream &os = std::clog);

	template <class T>
	class Property final : prop::detail::Property_name_base<T> {
		using prop::detail::Property_base::explicit_dependencies;
		using prop::detail::Property_base::need_update;
		using prop::detail::Property_base::read_notify;
		using prop::detail::Property_base::sever_implicit_dependents;
		using prop::detail::Property_base::update_complete;
		using prop::detail::Property_base::update_start;
		using prop::detail::Property_base::write_notify;
		//using prop::detail::Property_base::need_update;

		T value{};
		std::move_only_function<prop::Value(T &, const prop::detail::Binding_list &)> source;
		struct Write_notifier;

		public:
		Property();
		Property(Property<T> &&other);
		Property(prop::detail::Property_value_function<T> auto &&f);
		Property(prop::detail::Compatible_property_function<T> auto &&f);
		Property(prop::detail::Compatible_property<T> auto const &p);
		Property(prop::detail::Compatible_property<T> auto &p);
		Property(prop::detail::Compatible_property<T> auto &&p) = delete;
		Property(prop::detail::Property_value<T> auto &&v);
		Property(prop::detail::Property_function_binder<T> binder);

		Property &operator=(Property<T> &&other);
		Property &operator=(prop::detail::Property_update_function<T> auto &&f);
		Property &operator=(prop::detail::Compatible_property<T> auto &&p);
		Property &operator=(prop::detail::Property_value<T> auto &&v);
		Property &operator=(prop::detail::Property_function_binder<T> binder);
		void set(T t);
		const T &get() const;
		bool is_bound() const;
		void unbind() final override;
		void sever();
		Write_notifier apply();
		template <class Functor>
		std::invoke_result_t<Functor, T &> apply(Functor &&f);
		template <class Functor>
		std::invoke_result_t<Functor, T &> apply_guarded(Functor &&f);

		template <class U>
		bool is_implicit_dependency_of(const Property<U> &other) const;
		template <class U>
		bool is_explicit_dependency_of(const Property<U> &other) const;
		template <class U>
		bool is_dependency_of(const Property<U> &other) const;
		template <class U>
		bool is_implicit_dependent_of(const Property<U> &other) const;
		template <class U>
		bool is_explicit_dependent_of(const Property<U> &other) const;
		template <class U>
		bool is_dependent_on(const Property<U> &other) const;

		operator const T &() const;
		const T *operator->() const {
			this->read_notify();
			return &value;
		}
		const T &operator*() const {
			read_notify();
			return value;
		}

#define PROP_OP(OP)                                                                                                    \
	decltype(auto) operator OP() const                                                                                 \
		requires(requires { OP value; })                                                                               \
	{                                                                                                                  \
		read_notify();                                                                                                 \
		return OP value;                                                                                               \
	}                                                                                                                  \
	decltype(auto) operator OP()                                                                                       \
		requires(                                                                                                      \
			not requires { OP std::as_const(value); } and requires { OP value; })                                      \
	{                                                                                                                  \
		Write_notifier wn{this};                                                                                       \
		return OP value;                                                                                               \
	}

		//Unary ops: + - * & ~ ! ++ --
		PROP_OP(+)
		PROP_OP(-)
		PROP_OP(*)
		//PROP_OP(&) //don't forward, taking address of property should work as normal
		PROP_OP(~)
		PROP_OP(!)
		PROP_OP(++)
		PROP_OP(--)
#undef PROP_OP

		decltype(auto) operator++(int) const
			requires(requires { value++; })
		{
			read_notify();
			return value++;
		}
		decltype(auto) operator++(int)
			requires(
				not requires { std::as_const(value)++; } and requires { value++; })
		{
			Write_notifier wn{this};
			return value++;
		}

		decltype(auto) operator--(int) const
			requires(requires { value--; })
		{
			read_notify();
			return value--;
		}
		decltype(auto) operator--(int)
			requires(
				not requires { std::as_const(value)--; } and requires { value--; })
		{
			Write_notifier wn{this};
			return value--;
		}

#define PROP_OP(OP)                                                                                                    \
	template <class U>                                                                                                 \
	decltype(auto) operator OP(U &&u) const                                                                            \
		requires(requires { value OP std::forward<U>(u); })                                                            \
	{                                                                                                                  \
		read_notify();                                                                                                 \
		return value OP std::forward<U>(u);                                                                            \
	}                                                                                                                  \
	template <class U>                                                                                                 \
	decltype(auto) operator OP(U &&u)                                                                                  \
		requires(                                                                                                      \
			not requires { std::as_const(value) OP std::forward<U>(u); } and                                           \
			requires { value OP std::forward<U>(u); })                                                                 \
	{                                                                                                                  \
		Write_notifier wn{this};                                                                                       \
		return value OP std::forward<U>(u);                                                                            \
	}

		//Binary ops: + - * / % ^ | = < > += -= *= /= %= ^= &= |= << >> >>= <<= == != <= >= <=> && || , ->* ( ) [ ]
		PROP_OP(+)
		PROP_OP(-)
		PROP_OP(*)
		PROP_OP(/)
		PROP_OP(%)
		PROP_OP(^)
		PROP_OP(|)
		//PROP_OP(=) //done manually for more overloads
		PROP_OP(+=)
		PROP_OP(-=)
		PROP_OP(*=)
		PROP_OP(/=)
		PROP_OP(%=)
		PROP_OP(^=)
		PROP_OP(&=)
		PROP_OP(|=)
		PROP_OP(<<)
		PROP_OP(>>)
		PROP_OP(>>=)
		PROP_OP(<<=)
		//PROP_OP(<)
		//PROP_OP(>)
		//PROP_OP(==)
		//PROP_OP(!=)
		//PROP_OP(<=)
		//PROP_OP(>=)
		//PROP_OP(<=>)
		PROP_OP(&&)
		PROP_OP(||)
		//PROP_OP(,) //does not compile due to preprocessor limitations
		PROP_OP(->*)
		//PROP_OP(()) //variadic parameters
		//PROP_OP([]) //different syntax
#undef PROP_OP

		template <class U>
		decltype(auto) operator,(U &&u) const
			requires(requires { value, std::forward<U>(u); })
		{
			read_notify();
			return value, std::forward<U>(u);
		}
		template <class U>
		decltype(auto) operator,(U &&u)
			requires(
				not requires { std::as_const(value), std::forward<U>(u); } and requires { value, std::forward<U>(u); })
		{
			Write_notifier wn{this};
			return value, std::forward<U>(u);
		}

		template <class... Args>
		decltype(auto) operator()(Args &&...args) const
			requires(requires { value(std::forward<Args>(args)...); })
		{
			read_notify();
			return value(std::forward<Args>(args)...);
		}
		template <class... Args>
		decltype(auto) operator()(Args &&...args)
			requires(
				not requires { std::as_const(value)(std::forward<Args>(args)...); } and
				requires { value(std::forward<Args>(args)...); })
		{
			Write_notifier wn{this};
			return value(std::forward<Args>(args)...);
		}

		decltype(auto) operator[](std::convertible_to<std::size_t> auto &&u) const
			requires(requires { value[std::forward<decltype(u)>(static_cast<std::size_t>(u))]; })
		{
			read_notify();
			return value[std::forward<decltype(u)>(u)];
		}
		decltype(auto) operator[](std::convertible_to<std::size_t> auto &&u)
			requires(
				not requires { std::as_const(value)[std::forward<decltype(u)>(u)]; } and
				requires { value[std::forward<decltype(u)>(static_cast<std::size_t>(u))]; })
		{
			Write_notifier wn{this};
			return value[std::forward<decltype(u)>(u)];
		}

		template <class... Args>
		decltype(auto) operator[](Args &&...args) const
			requires(requires { value[std::forward<Args>(args)...]; } and sizeof...(Args) != 1)
		{
			read_notify();
			return value[std::forward<Args>(args)...];
		}
		template <class... Args>
		decltype(auto) operator[](Args &&...args)
			requires(
				requires { value[std::forward<Args>(args)...]; } and
				not requires { std::as_const(value)[std::forward<Args>(args)...]; } and sizeof...(Args) != 1)
		{
			read_notify();
			return value[std::forward<Args>(args)...];
		}

#ifdef PROPERTY_NAMES
		using prop::detail::Property_base::custom_name;
#endif

		private:
		struct Write_notifier {
			decltype(auto) operator->() && {
				if constexpr (prop::detail::has_operator_arrow_v<decltype(p->value)>) {
					return static_cast<decltype(p->value) &>(p->value);
				} else {
					return &p->value;
				}
			}
			~Write_notifier() {
				if (p) {
					p->write_notify();
				}
			}

			private:
			Write_notifier(prop::Property<T> *p_)
				: p{p_} {}
			friend class prop::Property<T>;
			prop::Property<T> *p;
		};
		void update_source(std::move_only_function<prop::Value(T &, const prop::detail::Binding_list &)> f);
		void update() override final;
		friend class Binding;
		friend prop::detail::Property_base *detail::get_property_base_pointer<T>(const Property &p);
		friend prop::detail::Property_base *detail::get_property_base_pointer<T>(const Property *p);
		template <class T_, class Function, class... Properties, std::size_t... indexes>
			requires(not std::is_same_v<T_, void>)
		friend std::move_only_function<prop::Value(T_ &, const prop::detail::Binding_list &)>
		prop::detail::create_explicit_caller(Function &&function, std::index_sequence<indexes...>);
		template <class T_, class Function, class... Properties, std::size_t... indexes>
			requires(std::is_same_v<T_, void>)
		friend std::move_only_function<void(const prop::detail::Binding_list &)>
		prop::detail::create_explicit_caller(Function &&function, std::index_sequence<indexes...>);
		template <class U>
		friend void prop::print_status(const prop::Property<U> &p, std::ostream &os);
		template <class U>
		friend class Property;
	};

	void print_status(const prop::Property<void> &p, std::ostream &os = std::clog);

	template <>
	class Property<void> final : prop::detail::Property_name_base<prop::Property<void>> {
		public:
		Property();
		Property(std::convertible_to<std::move_only_function<void()>> auto &&f);
		Property &operator=(std::convertible_to<std::move_only_function<void()>> auto &&f);
		Property &operator=(prop::detail::Property_function_binder<void> binder);
		template <class... Property_types,
				  prop::detail::Property_update_function<void, const Property<Property_types> *...> Function>
		void bind(Function &&source, Property<Property_types> &...properties);
		bool is_bound() const;
		void unbind() final override;

		template <class U>
		bool is_implicit_dependency_of(const Property<U> &other) const;
		template <class U>
		bool is_explicit_dependency_of(const Property<U> &other) const;
		template <class U>
		bool is_dependency_of(const Property<U> &other) const;
		template <class U>
		bool is_implicit_dependent_of(const Property<U> &other) const;
		template <class U>
		bool is_explicit_dependent_of(const Property<U> &other) const;
		template <class U>
		bool is_dependent_on(const Property<U> &other) const;
#ifdef PROPERTY_NAMES
		using prop::detail::Property_base::custom_name;
#endif

		private:
		void update_source(std::move_only_function<void(const prop::detail::Binding_list &)> f);
		void update() override final;
		std::move_only_function<void(const prop::detail::Binding_list &)> source;
		friend class Binding;
		friend Property_base *detail::get_property_base_pointer<void>(const Property &p);
		friend Property_base *detail::get_property_base_pointer<void>(const Property *p);
		template <class T, class Function, class... Properties, std::size_t... indexes>
			requires(not std::is_same_v<T, void>)
		friend std::move_only_function<prop::Value(T &, const prop::detail::Binding_list &)>
		prop::detail::create_explicit_caller(Function &&function, std::index_sequence<indexes...>);
		template <class T, class Function, class... Properties, std::size_t... indexes>
			requires(std::is_same_v<T, void>)
		friend std::move_only_function<void(const prop::detail::Binding_list &)>
		prop::detail::create_explicit_caller(Function &&function, std::index_sequence<indexes...>);
		friend void prop::print_status(const prop::Property<void> &p, std::ostream &os);
		template <class U>
		friend class Property;
	};

	template <class U>
	bool Property<void>::is_implicit_dependency_of(const Property<U> &other) const {
		return prop::detail::Property_base::is_implicit_dependency_of(other);
	}

	template <class U>
	bool Property<void>::is_explicit_dependency_of(const Property<U> &other) const {
		return prop::detail::Property_base::is_explicit_dependency_of(other);
	}

	template <class U>
	bool Property<void>::is_dependency_of(const Property<U> &other) const {
		return prop::detail::Property_base::is_dependency_of(other);
	}

	template <class U>
	bool Property<void>::is_implicit_dependent_of(const Property<U> &other) const {
		return prop::detail::Property_base::is_implicit_dependent_of(other);
	}

	template <class U>
	bool Property<void>::is_explicit_dependent_of(const Property<U> &other) const {
		return prop::detail::Property_base::is_explicit_dependent_of(other);
	}

	template <class U>
	bool Property<void>::is_dependent_on(const Property<U> &other) const {
		return prop::detail::Property_base::is_dependent_on(other);
	}

	namespace detail {
		template <class T>
		T inner_value_type(prop::Property<T> &&);
		template <class T>
		T inner_value_type(T &&t);

		template <class T>
		using inner_value_type_t =
			std::remove_cvref_t<decltype(prop::detail::inner_value_type(std::declval<std::remove_cvref_t<T>>()))>;

		template <class T>
		using inner_function_type_t = inner_value_type_t<decltype(std::declval<T>()())>;

		template <class T>
		inner_function_type_t<T> inner_type(std::true_type);
		template <class T>
		inner_value_type_t<T> inner_type(std::false_type);

		template <class T>
		auto is_function_type(T &&) -> decltype(std::declval<T>()(), void(0), std::true_type{});
		std::false_type is_function_type(...);

		template <class T>
		constexpr bool is_function_type_v = decltype(is_function_type(std::declval<T>()))::value;

		template <class T>
		using inner_type_t = decltype(inner_type<T>(is_function_type(std::declval<T>())));

	} // namespace detail

	template <class T>
	Property<T>::Property()
		: value{} {}

	template <class T>
	Property<T>::Property(Property<T> &&other)
		: value{std::move(other.value)}
		, source{std::move(other.source)} {
		static_cast<prop::detail::Property_base &>(*this) = static_cast<prop::detail::Property_base &&>(other);
	}

	template <class T>
	Property<T>::Property(prop::detail::Property_function_binder<T> binder) {
		*this = std::move(binder);
	}

	template <class T>
	Property<T> &Property<T>::operator=(Property<T> &&other) {
		value = std::move(other.value);
		source = std::move(other.source);
		static_cast<prop::detail::Property_base &>(*this) = static_cast<prop::detail::Property_base &&>(other);
		return *this;
	}

	template <class T>
	Property<T>::Property(prop::detail::Property_value_function<T> auto &&f)
		: value{[&f, this] {
			prop::detail::RAII updater{[this] { update_start(); }, [this] { update_complete(); }};
			return f();
		}()}
		, source{prop::detail::make_direct_update_function<T>(std::forward<decltype(f)>(f))} {}
	template <class T>

	Property<T>::Property(prop::detail::Compatible_property_function<T> auto &&f)
		: value{[&f, this] {
			prop::detail::RAII updater{[this] { update_start(); }, [this] { update_complete(); }};
			return f();
		}()}
		, source{prop::detail::make_direct_update_function<T>(std::forward<decltype(f)>(f))} {}

	template <class T>
	Property<T>::Property(prop::detail::Compatible_property<T> auto const &p)
		: value(p.value) {
		p.read_notify();
	}

	template <class T>
	Property<T>::Property(prop::detail::Compatible_property<T> auto &p)
		: value{std::as_const(p.value)} {
		p.read_notify();
	}

	template <class T>
	Property<T>::Property(prop::detail::Property_value<T> auto &&v)
		: value{std::forward<decltype(v)>(v)} {}

	template <class T>
	Property<T> &Property<T>::operator=(prop::detail::Property_update_function<T> auto &&f) {
		return *this = prop::detail::Property_function_binder<T>{std::forward<decltype(f)>(f)};
	}

	template <class T>
	Property<T> &Property<T>::operator=(prop::detail::Compatible_property<T> auto &&p) {
		unbind();
		p.read_notify();
#ifdef __cpp_lib_forward_like
		using Forward_t = decltype(std::forward_like<decltype(p)>(p.value));
#else
		using Forward_t = decltype(prop::detail::forward_like<decltype(p)>(p.value));
#endif

		if constexpr (prop::detail::is_equal_comparable_v<T, Forward_t>) {
			if (not prop::detail::is_equal(value, static_cast<Forward_t>(p.value))) {
				Write_notifier wn{this};
				value = static_cast<Forward_t>(p.value);
			}
		} else {
			Write_notifier wn{this};
			value = static_cast<Forward_t>(p.value);
		}
		return *this;
	}

	template <class T>
	Property<T> &Property<T>::operator=(prop::detail::Property_value<T> auto &&v) {
		unbind();
		if constexpr (prop::detail::is_equal_comparable_v<T, decltype(v)>) {
			if (not prop::detail::is_equal(value, v)) {
				Write_notifier wn{this};
				value = std::forward<decltype(v)>(v);
			}
		} else {
			Write_notifier wn{this};
			value = std::forward<decltype(v)>(v);
		}
		return *this;
	}

	template <class T>
	Property<T> &Property<T>::operator=(prop::detail::Property_function_binder<T> binder) {
		prop::detail::Property_base::set_explicit_dependencies(std::move(binder.explicit_dependencies));
		update_source(std::move(binder.function));
		return *this;
	}

	template <class T>
	Property<T>::operator const T &() const {
		return get();
	}

	template <class T>
	void Property<T>::set(T t) {
		unbind();
		if (!detail::is_equal(value, t)) {
			Write_notifier wn{this};
			value = std::move(t);
		}
	}

	template <class T>
	const T &Property<T>::get() const {
		read_notify();
		return value;
	}

	template <class T>
	bool Property<T>::is_bound() const {
		return source != nullptr;
	}

	template <class T>
	void Property<T>::unbind() {
		source = nullptr;
		prop::detail::Property_base::unbind();
	}

	template <class T>
	void Property<T>::sever() {
		sever_implicit_dependents();
	}

	template <class T>
	Property<T>::Write_notifier Property<T>::apply() {
		return {this};
	}

	template <class T>
	template <class Functor>
	std::invoke_result_t<Functor, T &> Property<T>::apply(Functor &&f) {
		Write_notifier wn{this};
		read_notify();
		if constexpr (std::is_fundamental_v<T> || std::is_pointer_v<T>) {
			T t = value;
			try {
				auto retval = f(value);
				if (t == value) {
					wn.p = nullptr;
				}
				return retval;
			} catch (...) {
				if (t == value) {
					wn.p = nullptr;
				}
				throw;
			}
		} else {
			return f(value);
		}
	}

	template <class T>
	template <class U>
	bool Property<T>::is_implicit_dependency_of(const Property<U> &other) const {
		return prop::detail::Property_base::is_implicit_dependency_of(other);
	}
	template <class T>
	template <class U>
	bool Property<T>::is_explicit_dependency_of(const Property<U> &other) const {
		return prop::detail::Property_base::is_explicit_dependency_of(other);
	}
	template <class T>
	template <class U>
	bool Property<T>::is_dependency_of(const Property<U> &other) const {
		return prop::detail::Property_base::is_dependency_of(other);
	}
	template <class T>
	template <class U>
	bool Property<T>::is_implicit_dependent_of(const Property<U> &other) const {
		return prop::detail::Property_base::is_implicit_dependent_of(other);
	}
	template <class T>
	template <class U>
	bool Property<T>::is_explicit_dependent_of(const Property<U> &other) const {
		return prop::detail::Property_base::is_explicit_dependent_of(other);
	}
	template <class T>
	template <class U>
	bool Property<T>::is_dependent_on(const Property<U> &other) const {
		return prop::detail::Property_base::is_dependent_on(other);
	}

	template <class T>
	template <class Functor>
	std::invoke_result_t<Functor, T &> Property<T>::apply_guarded(Functor &&f) {
		Write_notifier wn{this};
		prop::detail::RAII guard{[] { prop::detail::Property_base::current_binding = nullptr; },
								 [binding = prop::detail::Property_base::current_binding] {
									 prop::detail::Property_base::current_binding = binding;
								 }};
		read_notify();
		if constexpr (std::is_fundamental_v<T> || std::is_pointer_v<T>) {
			T t = value;
			try {
				auto retval = f(value);
				if (t == value) {
					wn.p = nullptr;
				}
				return retval;
			} catch (...) {
				if (t == value) {
					wn.p = nullptr;
				}
				throw;
			}
		} else {
			return f(value);
		}
	}

	template <class T>
	void Property<T>::update_source(std::move_only_function<prop::Value(T &, const prop::detail::Binding_list &)> f) {
		std::swap(f, source);
		update();
	}

	template <class T>
	void Property<T>::update() {
		prop::detail::RAII updater{[this] {
									   update_start(); //
								   },
								   [this] {
									   update_complete();
									   need_update = false;
								   }};
		try {
			if (source(value, explicit_dependencies) == prop::Value::changed) {
				updater.early_exit();
				write_notify();
			}
		} catch (const prop::Property_expired &) {
			unbind();
		} catch (...) {
			unbind();
			throw;
		}
#if 0
		Write_notifier wn{this};
		auto call_source = [this] {
			prop::detail::RAII updater{[this] { update_start(); },
									   [this] {
										   update_complete();
										   need_update = false;
									   }};
			return source(explicit_dependencies);
		};
		if constexpr (prop::detail::is_equal_comparable_v<T>) {
			try {
				T t = call_source();
				if (prop::detail::is_equal(t, value)) {
					wn.p = nullptr;
				} else {
					value = std::move(t);
				}
			} catch (const prop::Property_expired &) {
				unbind();
			}
		} else {
			if constexpr (std::is_move_assignable_v<T>) {
				try {
					value = call_source();
				} catch (const prop::Property_expired &) {
					unbind();
				}
			} else {
				throw prop::Logic_error{"Trying to update a " + std::string{prop::type_name<T>()} +
										" which is not move-assignable"};
			}
		}
#endif
	}

	template <class T>
	void print_status(const prop::Property<T> &p, std::ostream &os) {
		const auto &static_text_color = prop::Console_text_color{prop::Color::static_text};
		os << static_text_color;
#ifdef PROPERTY_NAMES
		os << "Property " << p.get_name() << '\n';
#else
		os << "Property " << prop::Console_text_color{prop::Color::address} << &p << '\n';
#endif
		os << static_text_color << "\tvalue: " << prop::console_reset_text_color << prop::detail::Printer{p.value}
		   << "\n";
		os << static_text_color << "\tsource: " << prop::console_reset_text_color << (p.source ? "Yes" : "No") << "\n";
		os << static_text_color << "\tExplicit dependencies: [" << prop::console_reset_text_color
		   << p.explicit_dependencies << static_text_color << "]\n";
		os << static_text_color << "\tImplicit dependencies: [" << prop::console_reset_text_color
		   << p.get_implicit_dependencies() << static_text_color << "]\n";
		os << static_text_color << "\tDependents: [" << prop::console_reset_text_color << p.get_dependents()
		   << static_text_color << "]\n"
		   << prop::console_reset_text_color;
	}

	Property<void>::Property(std::convertible_to<std::move_only_function<void()>> auto &&f) {
		update_source([source_ = std::forward<decltype(f)>(f)](const prop::detail::Binding_list &) { source_(); });
	}

	Property<void> &Property<void>::operator=(std::convertible_to<std::move_only_function<void()>> auto &&f) {
		update_source([source_ = std::forward<decltype(f)>(f)](const prop::detail::Binding_list &) { source_(); });
		return *this;
	}

	template <class T>
	Property(T &&t) -> Property<detail::inner_type_t<T>>;

#define PROP_BINOPS PROP_X(<=>) PROP_X(==) PROP_X(!=) PROP_X(<) PROP_X(<=) PROP_X(>) PROP_X(>=)
#define PROP_X(OP)                                                                                                     \
	template <class T, class U>                                                                                        \
		requires(prop::is_type_specialization_v<T, prop::Property> and                                                 \
				 not prop::is_type_specialization_v<U, prop::Property>)                                                \
	auto operator OP(const T &lhs, const U &rhs)->decltype(lhs.get() OP rhs) {                                         \
		return lhs.get() OP rhs;                                                                                       \
	}                                                                                                                  \
	template <class T, class U>                                                                                        \
		requires(not prop::is_type_specialization_v<T, prop::Property> and                                             \
				 prop::is_type_specialization_v<U, prop::Property>)                                                    \
	auto operator OP(const T &lhs, const U &rhs)->decltype(lhs OP rhs.get()) {                                         \
		return lhs OP rhs.get();                                                                                       \
	}                                                                                                                  \
	template <class T, class U>                                                                                        \
		requires(prop::is_type_specialization_v<T, prop::Property> and                                                 \
				 prop::is_type_specialization_v<U, prop::Property>)                                                    \
	auto operator OP(const T &lhs, const U &rhs)->decltype(lhs.get() OP rhs.get()) {                                   \
		return lhs.get() OP rhs.get();                                                                                 \
	}
	PROP_BINOPS
#undef PROP_X
#undef PROP_BINOPS

} // namespace prop
