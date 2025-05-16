#pragma once

#include "callable.h"
#include "color.h"
#include "exceptions.h"
#include "proplib/utility/utility.h"
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
			// TODO: Make an attempt to compare containers properly
			return false;
		}

#ifdef PROPERTY_NAMES
		std::string to_string(const void *p);
#endif

		struct Property_base;

		struct Binding_set {
			private:
			struct Stable_iterator;

			public:
			bool has(const Property_base *) const;
			bool is_empty() const;
			bool add(Property_base *);
			void remove(Property_base *);
			void clear();
			void replace(Property_base *old_value, Property_base *new_value);
			Stable_iterator stable_iterator() const;
			std::set<Property_base *> set; //TODO: Try out flat_set or something to save some nanoseconds
		};

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

		void swap(Property_base &lhs, Property_base &rhs);

		struct Property_base {
			virtual void update();
			virtual void unbind();
			virtual std::string displayed_value() const;
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
			Property_base(const Binding_set &bs);
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
			mutable Binding_list explicit_dependencies;
			const Binding_set &get_implicit_dependencies() const;
			const Binding_set &get_dependents() const;
#ifdef PROPERTY_NAMES
			std::string_view type;
			std::string custom_name;
			std::string get_name() const {
				std::string auto_name = prop::to_string(prop::Color::type) + std::string{type} +
										prop::to_string(prop::Color::static_text) + "@" +
										prop::to_string(prop::Color::address) + prop::detail::to_string(this) +
										prop::to_string(prop::Color::reset);
				if (custom_name.empty()) {
					return auto_name;
				}
				return auto_name + ' ' + custom_name;
			}
#endif

			protected:
			~Property_base();

			private:
			mutable Binding_set implicit_dependencies;
			mutable Binding_set dependents;
			friend Dependency_tracer;
		};

		template <class T>
		struct Property_name_base : Property_base {
#ifdef PROPERTY_NAMES
			Property_name_base()
				: prop::detail::Property_base(prop::type_name<T>()) {}

			protected:
			~Property_name_base() = default;
#endif
		};

		struct Binding_set::Stable_iterator {
			private:
			struct Stable_iterator_base final : Property_base {
				using Property_base::Property_base;
			} base;
			std::vector<Property_base *>::const_iterator cit;

			public:
			Stable_iterator(const Binding_set &binding_set);
			Stable_iterator(const Stable_iterator &) = delete;
			operator bool() const;
			Stable_iterator &operator++();
			Property_base &operator*() const;
			Property_base *operator->() const;
		};

		template <class Property>
		constexpr bool is_property_v =
			prop::is_template_specialization_v<std::remove_cvref_t<Property>, prop::Property>;

		template <class Compatible_type, class Inner_property_type>
		concept Property_value =
			std::convertible_to<Compatible_type, Inner_property_type> && !is_property_v<Compatible_type>;

		template <class From, class To>
		concept assignable_to = std::is_assignable_v<To, From>;

		template <class Property, class Inner_property_type>
		concept Compatible_property =
			is_property_v<Property> && (!std::is_lvalue_reference_v<Property> || requires(Property &&p) {
				{ p.get() } -> Property_value<Inner_property_type>;
			});

		template <class Function, class T, class... Properties>
		concept Property_value_function = requires(Function &&f) {
			{ f(std::declval<Properties &>()...) } -> prop::detail::assignable_to<T &>;
		};

		template <class Function, class T, class... Properties>
		concept Value_result_function = requires(Function &&f) {
			{ f(std::declval<T &>(), std::declval<Properties &>()...) } -> std::same_as<prop::Value>;
		};

		template <class Function, class T, class... Properties>
		concept Property_update_function =
			Property_value_function<Function, T, Properties...> || Value_result_function<Function, T, Properties...>;

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

		template <class T, class Function_args_list, class Properties_list, std::size_t... indexes>
		consteval bool are_compatible_function_args_for_properties(std::index_sequence<indexes...>) {
			if constexpr (Function_args_list::size == Properties_list::size) {
				return (is_compatible_function_arg_for_property<typename Function_args_list::template at<indexes>,
																typename Properties_list::template at<indexes>>() and
						...);
			}
		}

		template <class T, class Function, class Properties_list, std::size_t... indexes>
		consteval bool is_viable_source_helper(Properties_list, std::index_sequence<indexes...>) {
			if constexpr (not prop::is_callable_v<Function>) {
				//not a callable
				return false;
			} else {
				using Function_info = prop::Callable_info_for<Function>;
				if constexpr (Properties_list::size == Function_info::Args::size) {
					//source function candidate
					if (not std::is_assignable_v<T &, typename Function_info::Return_type>) {
						//return type not assignable to value
						return false;
					}
					if (not(is_compatible_function_arg_for_property<
								typename Function_info::Args::template at<indexes>,
								typename Properties_list::template at<indexes>>() and
							...)) {
						//incompatible properties
						return false;
					}
					return true;
				} else if constexpr (Properties_list::size + 1 == Function_info::Args::size) {
					//value update candidate
					if (not std::is_same_v<typename Function_info::Return_type, prop::Value>) {
						//return type not prop::Value
						return false;
					}
					if (not std::is_constructible_v<typename Function_info::Args::template at<0>, T &>) {
						//first parameter must be compatible to T&
						return false;
					}
					if (not(is_compatible_function_arg_for_property<
								typename Function_info::Args::template at<indexes + 1>,
								typename Properties_list::template at<indexes>>() and
							...)) {
						//incompatible properties
						return false;
					}
					return true;
				}
				//mismatching number of arguments/parameters
				return false;
			}
		}

		template <class T, class Function, class... Properties>
		constexpr bool is_viable_source = is_viable_source_helper<T, Function>(
			prop::Type_list<Properties...>{}, std::index_sequence_for<Properties...>{});

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
	(prop::is_template_specialization_v<std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<Properties>>>,   \
										prop::Property> and                                                            \
	 ...)
		template <class T>
		struct Property_function_binder {
			template <class... Properties, class Function>
			Property_function_binder(Function &&function_, Properties &&...properties)
				requires prop::detail::is_viable_source<T, Function, Properties...>
				: function{create_explicit_caller<T, decltype(function_), Properties...>(
					  std::forward<decltype(function_)>(function_), std::index_sequence_for<Properties...>{})}
				, explicit_dependencies{{prop::detail::get_property_base_pointer(properties)...}} {}

			std::move_only_function<prop::Value(T &, const Binding_list &)> function;
			Binding_list explicit_dependencies;
		};

		template <>
		struct Property_function_binder<void> {
			template <class Function, class... Properties>
			Property_function_binder(Function &&function_, Properties &&...properties)
				requires PROP_ACTUALLY_PROPERTIES
				: function{create_explicit_caller<void, Function, Properties...>(
					  std::forward<Function>(function_), std::index_sequence_for<Properties...>{})}
				, explicit_dependencies{{prop::detail::get_property_base_pointer(properties)...}} {
				static_assert(prop::Callable_info_for<Function>::Args::size == sizeof...(Properties),
							  "Number of function arguments and properties don't match");
				static_assert(
					are_compatible_function_args_for_properties<void, typename prop::Callable_info_for<Function>::Args,
																prop::Type_list<Properties...>>(
						std::index_sequence_for<Properties...>{}),
					"Callable arguments and parameters are incompatible");
			}

			template <class Function, class... Properties>
				requires(not PROP_ACTUALLY_PROPERTIES)
			Property_function_binder(Function &&, Properties &&...) {
				static_assert(PROP_ACTUALLY_PROPERTIES,
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
		make_direct_update_function(Property_update_function<T> auto &&f) {
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
				return source(t);
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
		concept has_operator_arrow_v = requires(T &&t) { t.operator->(); } || std::is_pointer_v<T>;

		template <class... Args>
		struct Type_list {};

		template <class T>
		auto converts_to(T &) -> Type_list<>;
		template <class T>
		auto converts_to(const T &) -> Type_list<>;

		template <class From, class To, bool is_const_qualified>
		struct Converter {
			operator To()
				requires(not is_const_qualified and not std::is_reference_v<To>)
			{
				return static_cast<To>(static_cast<From &>(static_cast<prop::Property<From> &>(*this).apply()));
			}
			operator To() const
				requires(is_const_qualified)
			{
				return static_cast<To>(static_cast<const prop::Property<From> &>(*this).get());
			}
		};

		template <class, bool, class>
		struct Conversion_provider;
		template <class T, bool is_const_qualified, class... Conversions>
		struct Conversion_provider<T, is_const_qualified, Type_list<Conversions...>>
			: Converter<T, Conversions, is_const_qualified>... {
			static_assert(((is_const_qualified or not std::is_reference_v<Conversions>) and ...),
						  "Non-const conversion to reference not supported because it's would likely leak modfiyable "
						  "internal state");
			using Converter<T, Conversions, is_const_qualified>::operator Conversions...;
			Conversion_provider() = default;
			template <class... Args>
			Conversion_provider(Type_list<Args...>);

			protected:
			~Conversion_provider() = default;
		};

		std::ostream &operator<<(std::ostream &os, const prop::detail::Binding_set &set);
		std::ostream &operator<<(std::ostream &os, const prop::detail::Binding_list &list);
		template <class T>
		std::ostream &operator<<(std::ostream &os, prop::detail::Printer<T> &&printer) {
			return printer.print(os);
		}
	} // namespace detail
} // namespace prop
