#pragma once

#include "callable.h"
#include "color.h"
#include "proplib/utility/utility.h"
#include "type_list.h"
#include "type_name.h"
#include "type_traits.h"

#include <cassert>
#include <concepts>
#include <functional>
#include <iostream>
#include <vector>

#ifdef PROPERTY_NAMES
#include <string>
#endif

namespace prop {
	template <class T>
	class Property;

	enum struct Updater_result : char { unchanged, changed, sever };

	class Dependency_tracer;

	namespace detail {
		template <class T, class U = T>
		concept is_equal_comparable_v = requires(const T &t, const U &u) {
			//TODO: handle containers and tuple-likes
			{ t == u } -> std::convertible_to<bool>;
		};

		template <class T, class U>
		constexpr bool is_equal(const T &lhs, const U &rhs) {
			//TODO: handle containers and tuple-likes
			if constexpr (is_equal_comparable_v<T, U>) {
				return lhs == rhs;
			}
			return false;
		}

		struct Property_base;

		struct Property_link {
			Property_link(const Property_base *property, bool required)
				: data{reinterpret_cast<std::uintptr_t>(property) + required} {}
			bool is_required() const {
				return data & 1;
			}
			Property_base *get_pointer() const {
				return reinterpret_cast<Property_base *>(data & ~std::uintptr_t{1});
			}
			Property_base *operator->() const {
				return get_pointer();
			}
			Property_base &operator*() const {
				return *get_pointer();
			}
			operator Property_base *() const {
				return get_pointer();
			}
			template <class T>
			operator prop::Property<T> *() const;
			Property_link &operator=(const Property_base *property) {
				data = reinterpret_cast<std::uintptr_t>(property) | is_required();
				return *this;
			}

			private:
			std::uintptr_t data;
		};

		struct Stable_list;

		void swap(Property_base &lhs, Property_base &rhs);

		struct Extended_status_data {
			std::ostream &output = std::clog;
			std::string indent_with = "\t";
			int depth = 0;
		};

		struct Property_base {
			virtual void update() {
				if (this != current_binding) {
					update();
				}
			}
			virtual void unbind() {
				for (std::size_t i = 0; i < explicit_dependencies + implicit_dependencies; i++) {
					dependencies[i]->remove_dependent(*this);
				}
				dependencies.erase(std::begin(dependencies),
								   std::begin(dependencies) + explicit_dependencies + implicit_dependencies);
				explicit_dependencies = implicit_dependencies = 0;
			}
			virtual std::string displayed_value() const {
				return "<base>";
			}
			void read_notify() const;
			void write_notify();
			void update_start(Property_base *&previous_binding);
			void update_complete(Property_base *&previous_binding);

			Property_base(const Property_base &) = delete;
			Property_base(Property_base &&other);
			Property_base();
			Property_base(std::string_view type);
			Property_base(std::vector<prop::detail::Property_link> explicit_dependencies);
			void operator=(const Property_base &) = delete;
			void operator=(Property_base &&other);

			friend void swap(Property_base &lhs, Property_base &rhs);

			bool is_dependency_of(const Property_base &other) const {
				return other.has_dependency(*this);
			}
			bool is_implicit_dependency_of(const Property_base &other) const {
				return other.has_implicit_dependency(*this);
			}
			bool is_explicit_dependency_of(const Property_base &other) const {
				return other.has_explicit_dependency(*this);
			}
			bool is_implicit_dependent_of(const Property_base &other) const {
				return has_implicit_dependency(other);
			}
			bool is_explicit_dependent_of(const Property_base &other) const {
				return has_explicit_dependency(other);
			}
			bool is_dependent_on(const Property_base &other) const {
				return has_dependency(other);
			}
			//Dependency_list(std::vector<Property_link> explicit_dependencies_list = {})
			//	: dependencies{std::move(explicit_dependencies_list)}
			//	, explicit_dependencies{static_cast<long>(dependencies.size())} {}
			bool has_dependency(const Property_base &other) const {
				const auto end = std::begin(dependencies) + explicit_dependencies + implicit_dependencies;
				return std::find(std::begin(dependencies), end, &other) != end;
			}
			bool has_implicit_dependency(const Property_base &other) const {
				const auto end = std::begin(dependencies) + explicit_dependencies + implicit_dependencies;
				return std::find(std::begin(dependencies) + explicit_dependencies, end, &other) != end;
			}
			bool has_explicit_dependency(const Property_base &other) const {
				const auto end = std::begin(dependencies) + explicit_dependencies;
				return std::find(std::begin(dependencies), end, &other) != end;
			}
			bool has_dependent(const Property_base &other) const {
				return std::find(std::begin(dependencies) + explicit_dependencies + implicit_dependencies,
								 std::end(dependencies), &other) != std::end(dependencies);
			}
			void add_explicit_dependency(Property_link property) {
				dependencies.insert(std::begin(dependencies) + explicit_dependencies++, property);
				property->add_dependent(*this);
			}
			void add_implicit_dependency(Property_link property) {
				if (not has_dependency(*property)) {
					dependencies.insert(std::begin(dependencies) + explicit_dependencies + implicit_dependencies++,
										property);
					property->add_dependent(*this);
				}
			}
			void set_explicit_dependencies(std::vector<Property_link> deps);
			void replace_dependency(const Property_base &old_value, const Property_base &new_value) {
				for (auto it = std::begin(dependencies),
						  end = std::begin(dependencies) + explicit_dependencies + implicit_dependencies;
					 it != end; ++it) {
					if (*it == &old_value) {
						old_value.remove_dependent(*this);
						*it = &new_value;
						new_value.add_dependent(*this);
					}
				}
			}
			std::span<const Property_link> get_explicit_dependencies() const {
				return std::span{dependencies}.subspan(0, static_cast<std::size_t>(explicit_dependencies));
			}
			std::span<const Property_link> get_implicit_dependencies() const {
				return std::span{dependencies}.subspan(static_cast<std::size_t>(explicit_dependencies),
													   static_cast<std::size_t>(implicit_dependencies));
			}
			std::span<const Property_link> get_dependencies() const {
				return std::span{dependencies}.subspan(
					0, static_cast<std::size_t>(explicit_dependencies + implicit_dependencies));
			}
			std::span<const Property_link> get_dependents() const {
				return std::span{dependencies}.subspan(
					static_cast<std::size_t>(explicit_dependencies + implicit_dependencies));
			}

			Stable_list get_stable_dependents() const;

			virtual std::string_view type() const = 0;
			virtual std::string value_string() const = 0;
			virtual bool has_source() const = 0;
			void print_status(const Extended_status_data &esd = {}) const;
#ifdef PROPERTY_NAMES
			std::string custom_name;
			std::string get_name() const {
				std::string auto_name =
					prop::to_string(prop::Color::type) + std::string{type()} +
					prop::to_string(prop::Color::static_text) + "@" + prop::to_string(prop::Color::address) +
					prop::to_string(static_cast<const void *>(this)) + prop::to_string(prop::Color::reset);
				if (custom_name.empty()) {
					return auto_name;
				}
				return auto_name + ' ' + custom_name;
			}
#endif
			protected:
			~Property_base();

			private:
			void print_extended_status(const Extended_status_data &esd, int current_depth) const;
			void add_dependent(const Property_base &other) const {
				if (not has_dependent(other)) {
					dependencies.push_back({&other, false});
				}
			}
			void remove_dependent(const Property_base &other) const {
				for (auto it = std::begin(dependencies) + explicit_dependencies + implicit_dependencies;
					 it != std::end(dependencies); ++it) {
					if (*it == &other) {
						dependencies.erase(it);
						return;
					}
				}
			}
			void replace_dependent(const Property_base &old_value, const Property_base &new_value) {
				for (std::size_t dependent_index = explicit_dependencies + implicit_dependencies;
					 dependent_index < std::size(dependencies); ++dependent_index) {
					if (dependencies[dependent_index] == &old_value) {
						dependencies[dependent_index] = &new_value;
						break;
					}
				}
			}

			mutable std::vector<Property_link> dependencies;
			mutable std::uint16_t explicit_dependencies = 0;
			mutable std::uint16_t implicit_dependencies = 0;
			static inline Property_base *current_binding;

			friend Dependency_tracer;
			friend Stable_list;
		};
		static_assert(alignof(Property_base) >= 2, "The last bit of any Property_base * is assumed to always be 0");

		struct Stable_list final : private Property_base {
			private:
			struct Stable_list_iterator {
				Stable_list_iterator(const Stable_list *stable_list, std::size_t pos = 0)
					: list{stable_list}
					, index{pos} {}
				Stable_list_iterator(const Stable_list_iterator &) = delete;
				operator bool() const {
					return index < list->explicit_dependencies + list->implicit_dependencies;
				}
				Stable_list_iterator &operator++() {
					while (*this and not list->dependencies[++index]) {
					}
					return *this;
				}
				Stable_list_iterator operator++(int) {
					auto pos = index;
					++*this;
					return Stable_list_iterator{list, pos};
				}
				Property_link &operator*() const {
					return list->dependencies[index];
				}
				Property_link *operator->() const {
					return &list->dependencies[index];
				}
				bool operator==(std::nullptr_t) const {
					return *this;
				}

				private:
				const Stable_list *list;
				std::size_t index = 0;
			};

			using Property_base::dependencies;
			using Property_base::explicit_dependencies;
			using Property_base::implicit_dependencies;

			public:
			Stable_list(std::span<const Property_link> list)
				: Property_base{{std::begin(list), std::end(list)}} {}
			auto begin() const {
				return Stable_list_iterator{this};
			}
			auto end() const {
				return nullptr;
			}
			std::string_view type() const override {
				return "Stable_list";
			}
			std::string value_string() const override {
				return "Stable_list";
			}
			bool has_source() const override {
				return false;
			}
		};

		inline Stable_list Property_base::get_stable_dependents() const {
			return {get_dependents()};
		}

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
		concept Generator_function = requires(Function &&f) {
			{ f(std::declval<Properties &>()...) } -> prop::detail::assignable_to<T &>;
		};

		template <class Function, class T, class... Properties>
		concept Updater_function = requires(Function &&f) {
			{
				f(std::declval<prop::Property<T> &>(), std::declval<Properties &>()...)
			} -> std::same_as<prop::Updater_result>;
		};

		template <class Function, class T, class... Properties>
		concept Property_update_function =
			Generator_function<Function, T, Properties...> || Updater_function<Function, T, Properties...>;

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
				if constexpr (Properties_list::size == Function_info::Params::size) {
					//source function candidate
					if (not std::is_assignable_v<T &, typename Function_info::Return_type>) {
						//return type not assignable to value
						return false;
					}
					if (not(is_compatible_function_arg_for_property<
								typename Function_info::Params::template at<indexes>,
								typename Properties_list::template at<indexes>>() and
							...)) {
						//incompatible properties
						return false;
					}
					return true;
				} else if constexpr (Properties_list::size + 1 == Function_info::Params::size) {
					//value update candidate
					if (not std::is_same_v<typename Function_info::Return_type, prop::Updater_result>) {
						//return type not prop::Updater_result
						return false;
					}
					if (not std::is_constructible_v<typename Function_info::Params::template at<0>, T &>) {
						//first parameter must be compatible to T&
						return false;
					}
					if (not(is_compatible_function_arg_for_property<
								typename Function_info::Params::template at<indexes + 1>,
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

		template <class Parameter, class Argument>
		struct Property_pass_traits;

		template <class Parameter, class Argument>
			requires(prop::is_template_specialization_v<
					 std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<Argument>>>, prop::Property>)
		struct Property_pass_traits<Parameter, Argument> {
			using Property_type = std::remove_pointer_t<std::remove_reference_t<Argument>>;
			using Value_type = Property_type::Value_type;
			constexpr static bool argument_is_property = true;
			constexpr static bool argument_is_const = std::is_const_v<Property_type>;
			constexpr static bool parameter_is_prop = prop::is_template_specialization_v<
				std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<Parameter>>>, prop::Property>;
			constexpr static bool argument_is_required =
				argument_is_property ? not std::is_pointer_v<Parameter> :
									   not std::is_same_v<Parameter, std::add_pointer_t<Value_type>>;
			constexpr static bool argument_is_readonly = std::is_constructible_v<Argument, const Property_type &> or
														 std::is_constructible_v<Argument, const Property_type *> or
														 std::is_constructible_v<Argument, const Value_type &> or
														 std::is_constructible_v<Argument, const Value_type *>;
			constexpr static bool argument_matches_parameter = std::is_constructible_v<Argument, Property_type &> or
															   std::is_constructible_v<Argument, Property_type *> or
															   std::is_constructible_v<Argument, Value_type &> or
															   std::is_constructible_v<Argument, Value_type *>;
		};
		template <class Parameter, class Argument>
			requires(not prop::is_template_specialization_v<
					 std::remove_cvref_t<std::remove_pointer_t<std::remove_cvref_t<Argument>>>, prop::Property>)
		struct Property_pass_traits<Parameter, Argument> {
			constexpr static bool argument_is_property = false;
		};

		template <class Function_arg, class Property>
		decltype(auto) convert_to_function_arg(const Property *property) {
			using T = std::remove_cvref_t<decltype(property->get())>;
			if constexpr (std::is_constructible_v<Function_arg, const Property &>) {
				assert(property);
				return *property;
			} else if constexpr (std::is_constructible_v<Function_arg, const Property *>) {
				return property;
			} else if constexpr (std::is_constructible_v<Function_arg, Property &>) {
				assert(property);
				return *const_cast<Property *>(property);
			} else if constexpr (std::is_constructible_v<Function_arg, Property *>) {
				return const_cast<Property *>(property);
			} else if constexpr (std::is_constructible_v<Function_arg, const T &>) {
				assert(property);
				return property->get();
			} else if constexpr (std::is_constructible_v<Function_arg, const T *>) {
				return property ? &property->get() : nullptr;
			} else if constexpr (std::is_constructible_v<Function_arg, T &>) {
				assert(property);
				return const_cast<T &>(property->get());
			} else if constexpr (std::is_constructible_v<Function_arg, T *>) {
				return property ? &const_cast<T &>(property->get()) : nullptr;
			}
		}

		template <class... Params, class... Args>
		constexpr std::string type_comparer(const bool (&highlights)[sizeof...(Params)], prop::Type_list<Params...>,
											prop::Type_list<Args...>) {
			std::vector<std::string> args = {std::string{prop::type_name<std::remove_reference_t<Args>>()}...};
			std::vector<std::string> params = {std::string{prop::type_name<std::remove_reference_t<Params>>()}...};
			std::string argss = "Arguments:  ";
			std::string paramss = "Parameters: ";
			for (std::size_t i = 0; i < sizeof...(Params); i++) {
				if (i) {
					for (auto &s : {&argss, &paramss}) {
						*s += ", ";
					}
				}
				auto length = std::max(args[i].size(), params[i].size());
				for (auto &s : {&args[i], &params[i]}) {
					while (s->size() < length) {
						s->push_back(' ');
					}
				}
				constexpr auto highlight = [](std::string s) { return "\033[1;38;2;255;40;0m" + s + "\033[0m"; };
				argss += highlights[i] ? highlight(args[i]) : args[i];
				paramss += highlights[i] ? highlight(params[i]) : params[i];
			}
			return paramss + '\n' + argss + '\n';
		};

		template <class T, class Function, class... Properties, std::size_t... indexes>
			requires(not std::is_same_v<T, void>)
		std::move_only_function<prop::Updater_result(prop::Property<T> &,
													 std::span<const Property_link> explicit_dependencies)>
		create_explicit_caller(Function &&function, std::index_sequence<indexes...>) {
			return [source = std::forward<Function>(function)](
					   prop::Property<T> &p,
					   std::span<const Property_link> explicit_dependencies) mutable -> prop::Updater_result {
				using Function_parameter_list = prop::Callable_info_for<Function>::Params;
				if constexpr (Function_parameter_list::size == sizeof...(indexes)) {
#define PROP_ARGS                                                                                                      \
	convert_to_function_arg<typename Function_parameter_list::template at<indexes>>(                                   \
		static_cast<std::remove_pointer_t<std::remove_cvref_t<Properties>> *>(explicit_dependencies[indexes]))
					if constexpr (prop::detail::is_equal_comparable_v<T, decltype(source(PROP_ARGS...))>) {
						auto value = source(PROP_ARGS...);
						if (prop::detail::is_equal(p, value)) {
							return prop::Updater_result::unchanged;
						} else {
							p = std::move(value);
							return prop::Updater_result::changed;
						}
					} else {
						p = source(PROP_ARGS...);
						return prop::Updater_result::changed;
					}
#undef PROP_ARGS
				} else if constexpr (Function_parameter_list::size == sizeof...(indexes) + 1) {
					constexpr bool conversion_failures[] = {
						std::is_same_v<
							void, decltype(convert_to_function_arg<typename Function_parameter_list::template at<0>>(
									  static_cast<prop::Property<T> *>(nullptr)))>,
						std::is_same_v<void, decltype(convert_to_function_arg<
													  typename Function_parameter_list::template at<indexes + 1>>(
												 static_cast<std::remove_pointer_t<std::remove_cvref_t<Properties>> *>(
													 nullptr)))>...,
					};
					static_assert(not conversion_failures[0] and (... and not conversion_failures[indexes + 1]),
								  "Updater function cannot be called due to type mismatch:\n" +
									  type_comparer(conversion_failures, Function_parameter_list{},
													prop::Type_list<prop::Property<T>, Properties...>{}));
					static_assert(
						std::is_same_v<typename prop::Callable_info_for<Function>::Return_type, prop::Updater_result>,
						"Updating function must return prop::Updater_result, got " +
							std::string{prop::type_name<typename prop::Callable_info_for<Function>::Return_type>()} +
							" instead.");
					return source(convert_to_function_arg<typename Function_parameter_list::template at<0>>(&p),
								  convert_to_function_arg<typename Function_parameter_list::template at<indexes + 1>>(
									  static_cast<std::remove_pointer_t<std::remove_cvref_t<Properties>> *>(
										  explicit_dependencies[indexes]))...);
				} else {
					static_assert(false, "Number of parameters (" + std::string(Function_parameter_list::type_names) +
											 ") does not match number of given properties (" +
											 std::to_string(prop::Type_list<Properties...>::type_names) + ")");
				}
			};
		}

		template <class T, class Function, class... Properties, std::size_t... indexes>
			requires(std::is_same_v<T, void>)
		std::move_only_function<void(std::span<const Property_link>)>
		create_explicit_caller(Function &&function, std::index_sequence<indexes...>) {
			return [source = std::forward<Function>(function)](
					   std::span<const Property_link> explicit_dependencies) mutable {
				using Args_list = prop::Callable_info_for<Function>::Params;
				if constexpr (Args_list::size == sizeof...(indexes)) {
					source(convert_to_function_arg<typename Args_list::template at<indexes>>(
						static_cast<std::remove_pointer_t<std::remove_cvref_t<Properties>> *>(
							explicit_dependencies[indexes]))...);
				} else {
					using Function_parameter_list = prop::Callable_info_for<Function>::Params;
					static_assert(false, "Number of parameters (" + std::string(Function_parameter_list::type_names) +
											 ") does not match number of given properties (" +
											 std::to_string(prop::Type_list<Properties...>::type_names) + ")");
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
				requires prop::is_callable_v<Function>
			Property_function_binder(Function &&function_, Properties &&...properties)
				//requires prop::detail::is_viable_source<T, Function, Properties...>
				: Property_function_binder(std::forward<Function>(function_),
										   typename prop::Callable_info_for<Function>::Params{}, properties...) {}

			std::move_only_function<prop::Updater_result(prop::Property<T> &, std::span<const Property_link>)> function;
			std::vector<prop::detail::Property_link> dependencies;

			private:
			template <class... Properties, class Function, class... Parameters>
				requires(sizeof...(Properties) == sizeof...(Parameters))
			Property_function_binder(Function &&function_, prop::Type_list<Parameters...>, Properties &&...properties)
				: function{create_explicit_caller<T, decltype(function_), Properties...>(
					  std::forward<decltype(function_)>(function_), std::index_sequence_for<Properties...>{})}
				, dependencies{{prop::detail::get_property_base_pointer(properties),
								Property_pass_traits<Parameters, Properties>::argument_is_required}...} {
				static_assert(PROP_ACTUALLY_PROPERTIES, "Must pass properties");
			}
			template <class... Properties, class Function, class First_parameter, class... Parameters>
				requires(sizeof...(Properties) == sizeof...(Parameters))
			Property_function_binder(Function &&function_, prop::Type_list<First_parameter, Parameters...>,
									 Properties &&...properties)
				: function{create_explicit_caller<T, decltype(function_), Properties...>(
					  std::forward<decltype(function_)>(function_), std::index_sequence_for<Properties...>{})}
				, dependencies{{prop::detail::get_property_base_pointer(properties),
								Property_pass_traits<Parameters, Properties>::argument_is_required}...} {
				static_assert(PROP_ACTUALLY_PROPERTIES, "Must pass properties");
			}
		};

		template <>
		struct Property_function_binder<void> {
			template <class... Properties, class Function>
				requires prop::is_callable_v<Function>
			Property_function_binder(Function &&function_, Properties &&...properties)
				//requires prop::detail::is_viable_source<T, Function, Properties...>
				: Property_function_binder(std::forward<Function>(function_),
										   typename prop::Callable_info_for<Function>::Params{}, properties...) {}

			std::move_only_function<void(std::span<const Property_link>)> function;
			std::vector<prop::detail::Property_link> dependencies;

			private:
			template <class... Properties, class Function, class... Parameters>
				requires prop::is_callable_v<Function>
			Property_function_binder(Function &&function_, prop::Type_list<Parameters...>, Properties &&...properties)
				//requires prop::detail::is_viable_source<T, Function, Properties...>
				: function{create_explicit_caller<void, decltype(function_), Properties...>(
					  std::forward<decltype(function_)>(function_), std::index_sequence_for<Properties...>{})}
				, dependencies{{prop::detail::get_property_base_pointer(properties),
								Property_pass_traits<Parameters, Properties>::argument_is_required}...} {
				static_assert(PROP_ACTUALLY_PROPERTIES, "Must pass properties");
			}
		};
#undef PROP_ACTUALLY_PROPERTIES

		template <class T>
		std::move_only_function<prop::Updater_result(prop::Property<T> &, std::span<const Property_link>)>
		make_direct_update_function(Generator_function<T> auto &&f) {
			return
				[source = std::forward<decltype(f)>(f)](prop::Property<T> &t, std::span<const Property_link>) mutable {
					auto value = source();
					if (is_equal(value, t)) {
						return prop::Updater_result::unchanged;
					}
					t = std::move(value);
					return prop::Updater_result::changed;
				};
		}

		template <class T>
		std::move_only_function<prop::Updater_result(prop::Property<T> &, std::span<const Property_link>)>
		make_direct_update_function(Property_update_function<T> auto &&f) {
			return
				[source = std::forward<decltype(f)>(f)](prop::Property<T> &t, std::span<const Property_link>) mutable {
					if constexpr (prop::detail::is_equal_comparable_v<std::decay_t<decltype(source())>, T>) {
						auto value = source();
						if (is_equal(value, t)) {
							return prop::Updater_result::unchanged;
						}
						t = std::move(value);
						return prop::Updater_result::changed;
					} else {
						t = source();
						return prop::Updater_result::changed;
					}
				};
		}

		template <class T>
		std::move_only_function<prop::Updater_result(prop::Property<T> &, std::span<const Property_link>)>
		make_direct_update_function(Updater_function<T> auto &&f) {
			return [source = std::forward<decltype(f)>(f)](
					   prop::Property<T> &t, std::span<const Property_link>) mutable { return source(t); };
		}

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

		template <class From, class To>
			requires(not std::is_same_v<std::remove_cvref_t<From>, std::remove_cvref_t<To>>)
		struct Conversion_type {
			using T = std::remove_cvref_t<To>;
			template <class U>
			static constexpr bool has_operator =
				requires(From from) { from.operator U(); } and std::convertible_to<From, U>;
			static constexpr bool has_ref_operator = has_operator<T &>;
			static constexpr bool has_const_ref_operator = has_operator<const T &>;
			static constexpr bool has_temp_ref_operator = has_operator<T &&>;
			static constexpr bool has_const_temp_ref_operator = has_operator<const T &&>;
			static constexpr bool has_value_operator = has_operator<T>;
			static constexpr bool has_const_value_operator = has_operator<const T>;
			static constexpr bool has_value_cast =
				not(has_ref_operator or has_const_ref_operator or has_temp_ref_operator or
					has_const_temp_ref_operator or has_value_operator or has_const_value_operator) and
				(std::is_fundamental_v<T> or std::is_pointer_v<T>) and requires(From from) { static_cast<T>(from); };

			static constexpr bool pass_by_ref = has_ref_operator or has_const_ref_operator;
			static constexpr bool pass_by_temp_ref = has_temp_ref_operator or has_const_temp_ref_operator;
			static constexpr bool pass_by_const_value = has_const_value_operator;
			static constexpr bool pass_by_value = has_value_operator or (not pass_by_ref and not pass_by_temp_ref and
																		 not pass_by_const_value and has_value_cast);
			static constexpr bool valid_conversion =
				pass_by_ref or pass_by_temp_ref or pass_by_value or pass_by_const_value;
		};
	} // namespace detail
} // namespace prop
