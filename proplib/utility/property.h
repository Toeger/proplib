#pragma once

#include "exceptions.h"
#include "raii.h"
#include "type_name.h"
#include "type_traits.h"

#include <cassert>
#include <concepts>
#include <functional>
#include <set>
#include <tuple>

//#define PROPERTY_DEBUG

#ifdef PROPERTY_DEBUG
#include <string>
#endif

namespace prop {
	template <class T>
	class Property;
	namespace detail {
		struct Property_base;
		struct Binding_set {
			bool has(Property_base *) const;
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

		struct Binding_list {
			bool has(Property_base *property) const;
			std::size_t size() const;
			void replace(Property_base *old_value, Property_base *new_value);
			Property_base *operator[](std::size_t index) const;
			std::vector<Property_base *>::iterator begin();
			std::vector<Property_base *>::iterator end();
			std::vector<Property_base *>::const_iterator begin() const;
			std::vector<Property_base *>::const_iterator end() const;
			std::vector<Property_base *> list; //TODO: Try out other things like std::forward_list
		};

		struct Property_base {
			virtual void update();
			virtual void unbind();
			void unbind_depends();
			void read_notify() const;
			void read_notify();
			void write_notify();
			void update_start();
			void update_complete();

			Property_base();
			Property_base(Property_base &&other);
			Property_base(const Property_base &) = delete;
			void operator=(const Property_base &) = delete;
			~Property_base();

			bool need_update = false;
			static inline Property_base *current_binding;
			Property_base *previous_binding = nullptr;
			Property_base *creation_binding = current_binding;
			void clear_implicit_dependencies();
			void sever_implicit_dependents();
			void take_explicit_dependents(Property_base &&source);
#ifdef PROPERTY_DEBUG
			std::string name;
#endif
			Binding_list explicit_dependencies;
#ifndef PROPERTY_DEBUG
			private:
#endif
			Binding_set implicit_dependencies;
			Binding_set dependents;
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
			concept Property_property_function = requires(Function &&f) {
				{ f(std::declval<Properties>()...) } -> Compatible_property<T>;
			};
		} // namespace
		template <class Function, class T, class... Properties>
		concept Property_function = Property_value_function<Function, T, Properties...> ||
									Property_property_function<Function, T, Properties...>;
	} // namespace detail

	extern void (*on_property_severed)(detail::Property_base *severed, detail::Property_base *reason);

	template <class T>
	class Property : detail::Property_base {
		public:
		Property() = default;
		Property(detail::Property_function<T> auto f);
		Property(detail::Compatible_property<T> auto const &p);
		Property(detail::Compatible_property<T> auto &p);
		Property(detail::Compatible_property<T> auto &&p);
		Property(detail::Property_value<T> auto &&v);

		Property &operator=(detail::Property_function<T> auto f);
		Property &operator=(detail::Compatible_property<T> auto &&p);
		Property &operator=(detail::Property_value<T> auto &&v);
		operator const T &() const;
		void set(T t);
		const T &get() const;
		template <class... Property_types, detail::Property_function<T, const Property<Property_types> *...> Function>
		void bind(Function source, Property<Property_types> &...properties);
		template <class U>
		void bind(Property<U> &other)
			requires(std::is_assignable_v<T &, U>);
		void unbind() final override;
		template <class Functor>
		std::invoke_result_t<Functor, T &> apply(Functor &&f);

		private:
		void update_source(std::function<T()> f);
		void update() override final;
		T value{};
		std::function<T()> source;
		template <class Functor, class... Args, std::size_t... Indexes>
		static auto call_with_explicit_dependencies(Functor &f, std::tuple<Args...> *tuple,
													detail::Binding_list &explicit_dependencies,
													std::index_sequence<Indexes...>) {
			assert(std::size(explicit_dependencies) == sizeof...(Indexes));
			return f(static_cast<std::remove_reference_t<decltype(std::get<Indexes>(*tuple))>>(
				explicit_dependencies[Indexes])...);
		}
	};

	namespace detail {
		template <class T, class U>
		auto is_equal_comparable(T &&t, U &&u) -> decltype(t == u, std::true_type{});
		std::false_type is_equal_comparable(...);
		template <class T, class U = T>
		constexpr bool is_equal_comparable_v =
			decltype(is_equal_comparable(std::declval<T>(), std::declval<U>()))::value;

		template <class T>
		constexpr bool is_equal(const T &lhs, const T &rhs) {
			if constexpr (is_equal_comparable_v<T>) {
				return lhs == rhs;
			}
			// TODO: Make an attempt to compare containers
			return false;
		}

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
		auto inner_type(std::true_type) -> inner_function_type_t<T>;
		template <class T>
		inner_value_type_t<T> inner_type(std::false_type);

		template <class T>
		auto is_function_type(T &&) -> decltype(std::declval<T>()(), std::true_type{});
		std::false_type is_function_type(...);

		template <class T>
		constexpr bool is_function_type_v = decltype(is_function_type(std::declval<T>()))::value;

		template <class T>
		using inner_type_t = decltype(inner_type<T>(is_function_type(std::declval<T>())));

	} // namespace detail

	template <class T>
	Property<T>::Property(detail::Property_function<T> auto f)
		: value{[&f, this] {
			detail::RAII updater{[this] { update_start(); }, [this] { update_complete(); }};
			return f();
		}()}
		, source{std::move(f)} {}

	template <class T>
	Property<T>::Property(detail::Compatible_property<T> auto const &p)
		: value(p.value) {}

	template <class T>
	Property<T>::Property(detail::Compatible_property<T> auto &p)
		: value(p.value) {}

	template <class T>
	Property<T>::Property(detail::Compatible_property<T> auto &&p)
		: detail::Property_base{std::move(static_cast<Property_base &>(p))}
		, value(std::move(p.value))
		, source{std::move(p.source)} {}

	template <class T>
	Property<T>::Property(detail::Property_value<T> auto &&v)
		: value(std::forward<decltype(v)>(v)) {}

	template <class T>
	Property<T> &Property<T>::operator=(detail::Property_function<T> auto f) {
		update_source(std::move(f));
		return *this;
	}

	template <class T>
	Property<T> &Property<T>::operator=(detail::Compatible_property<T> auto &&p) {
		unbind();
		detail::RAII notifier{[this] { write_notify(); }};
		if constexpr (detail::is_equal_comparable_v<T>) {
			T t(std::forward<decltype(p)>(p).value);
			if (!detail::is_equal(t, value)) {
				value = std::move(t);
			} else {
				notifier.cancel();
			}
		} else {
			value = std::forward<decltype(p)>(p).value;
		}
		return *this;
	}

	template <class T>
	Property<T> &Property<T>::operator=(detail::Property_value<T> auto &&v) {
		unbind();
		detail::RAII notifier{[this] { write_notify(); }};
		if constexpr (detail::is_equal_comparable_v<T>) {
			T t(std::forward<decltype(v)>(v));
			if (!detail::is_equal(t, value)) {
				value = std::move(t);
			} else {
				notifier.cancel();
			}
		} else {
			value = std::forward<decltype(v)>(v);
		}
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
			detail::RAII notifier{[this] { write_notify(); }};
			value = std::move(t);
		}
	}

	template <class T>
	const T &Property<T>::get() const {
		read_notify();
		return value;
	}

	template <class T>
	template <class... Property_types, detail::Property_function<T, const Property<Property_types> *...> Function>
	void Property<T>::bind(Function source, Property<Property_types> &...properties) {
		explicit_dependencies = {{&properties...}};
		update_source([this, source = std::move(source)] {
			return call_with_explicit_dependencies(source, (std::tuple<const Property<Property_types> *...> *){},
												   explicit_dependencies, std::index_sequence_for<Property_types...>());
		});
	}

	template <class T>
	template <class U>
	void Property<T>::bind(Property<U> &other)
		requires(std::is_assignable_v<T &, U>)
	{
		bind(std::function<T(const Property<U> *)>{[this](const Property<U> *other) -> T {
				 if (!other) {
					 unbind();
					 return value;
				 }
				 return other->get();
			 }},
			 other);
	}

	template <class T>
	void Property<T>::unbind() {
		source = nullptr;
		Property_base::unbind();
	}

	template <class T>
	template <class Functor>
	std::invoke_result_t<Functor, T &> Property<T>::apply(Functor &&f) {
		detail::RAII notifier{[this] { write_notify(); }};
		read_notify();
		if constexpr (std::is_fundamental_v<T> || std::is_pointer_v<T>) {
			T t = value;
			try {
				auto retval = f(value);
				if (t == value) {
					notifier.cancel();
				}
				return retval;
			} catch (...) {
				if (t == value) {
					notifier.cancel();
				}
				throw;
			}
		} else {
			return f(value);
		}
	}

	template <class T>
	void Property<T>::update_source(std::function<T()> f) {
		std::swap(f, source);
		update();
	}

	template <class T>
	void Property<T>::update() {
		detail::RAII updater{[this] { update_start(); }, [this] { update_complete(); }};
		if constexpr (detail::is_equal_comparable_v<T>) {
			T t = source();
			updater.early_exit();
			need_update = false;
			if (!detail::is_equal(t, value)) {
				detail::RAII notifier{[this] { write_notify(); }};
				value = std::move(t);
			}
		} else {
			if constexpr (std::is_move_assignable_v<T>) {
				detail::RAII notifier{[this] { write_notify(); }};
				value = [this, &notifier] {
					try {
						return source();
					} catch (...) {
						notifier.cancel();
						throw;
					}
				}();
				updater.early_exit();
				need_update = false;
			} else {
				throw prop::Logic_error{"Trying to update a " + prop::type_name<T>() + " which is not move-assignable"};
			}
		}
	}

	template <class T>
	Property(T &&t) -> Property<detail::inner_type_t<T>>;

#define PROP_BINOPS PROP_X(<=>) PROP_X(==) PROP_X(!=) PROP_X(<) PROP_X(<=) PROP_X(>) PROP_X(>=)
#define PROP_X(OP)                                                                                                     \
	template <class T, class U>                                                                                        \
	auto operator OP(const Property<T> &lhs, const U &rhs) {                                                           \
		return lhs.get() OP rhs;                                                                                       \
	}                                                                                                                  \
	template <class T, class U>                                                                                        \
	auto operator OP(const U &lhs, const Property<T> &rhs) {                                                           \
		return lhs OP rhs.get();                                                                                       \
	}
	PROP_BINOPS
#undef PROP_X
#undef PROP_BINOPS

} // namespace prop
