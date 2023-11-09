#pragma once

#include "raii.h"
#include "type_traits.h"

#include <functional>
#include <set>

//#define PROPERTY_DEBUG

#ifdef PROPERTY_DEBUG
#include <string>
#endif

namespace prop {
	namespace detail {
		struct Property_base;
		struct Binding_set {
			bool has(Property_base *) const;
			bool is_empty() const;
			bool add(Property_base *);
			void remove(Property_base *);
			void clear();
			std::set<Property_base *>::iterator begin();
			std::set<Property_base *>::iterator end();
			std::set<Property_base *>::const_iterator begin() const;
			std::set<Property_base *>::const_iterator end() const;
			std::set<Property_base *>::const_iterator cbegin() const;
			std::set<Property_base *>::const_iterator cend() const;
			std::set<Property_base *> set; //TODO: Try out flat_set or something to save some nanoseconds
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
			Property_base(const Property_base &) = delete;
			void operator=(const Property_base &) = delete;
			~Property_base() noexcept(false);

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
			private:
			std::vector<Property_base *> explicit_depenencies;
			Binding_set implicit_dependencies;
			Binding_set dependents;
		};
	} // namespace detail

	extern void (*on_property_severed)(detail::Property_base *severed, detail::Property_base *reason);

	template <class T>
	class Property : detail::Property_base {
		public:
		Property() = default;
		template <class U>
		Property(U &&u);
		template <class U>
		Property &operator=(U &&rhs);
		operator const T &() const;
		void set(T t);
		const T &get() const;
		template <class U>
		void bind(const Property<U> &other);
		void bind(std::function<T()>);
		void unbind() final override;
		template <class Functor>
		std::invoke_result_t<Functor, T &> apply(Functor &&f);
#ifdef PROPERTY_DEBUG
		using detail::Property_base::name;
#endif
		private:
		void update_source(std::function<T()> f);
		void update() override final;

		T value{};
		std::function<T()> source;
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
		auto inner_value_type(Property<T> &&) -> T;
		template <class T>
		T inner_value_type(T &&t);

		template <class T>
		using inner_value_type_t =
			std::remove_cvref_t<decltype(inner_value_type(std::declval<std::remove_cvref_t<T>>()))>;

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
	template <class U>
	Property<T>::Property(U &&u)
		: value{[&u, this]() -> T {
			//move constructor //TODO: change this to std::is_move_assignable_v<base_of<U>, T>
			if constexpr (std::is_same_v<decltype(u), Property<T> &&>) {
				return std::move(u.value);
			}
			//function binding
			else if constexpr (detail::is_function_type_v<U>) {
				detail::RAII updater{[this] { update_start(); }, [this] { update_complete(); }};
				return u();
			}
			//value assignments
			else if constexpr (std::is_convertible_v<decltype(std::forward<U>(u)), T>) {
				return std::forward<U>(u);
			} else if constexpr (prop::is_template_specialization_v<std::remove_cvref_t<U>, prop::Property>) {
				if constexpr (std::is_rvalue_reference_v<U &&>) {
					u.read_notify();
					return std::move(u.value);
				} else {
					return u.value;
				}
			}
			//error
			else {
				static_assert(decltype(u, std::false_type{})::value, "Invalid assignment");
			}
		}()}
		, source{[&]() -> std::function<T()> {
			//move constructor //TODO: change this to std::is_move_assignable_v<base_of<U>, T>
			if constexpr (std::is_same_v<decltype(u), Property<T> &&>) {
				return std::move(u.source);
			}
			//function binding
			if constexpr (detail::is_function_type_v<U>) {
				if constexpr (std::is_convertible_v<std::invoke_result_t<U>, T>) {
					return std::forward<U>(u);
				} else {
					static_assert(decltype(u, std::false_type{})::value,
								  "Invalid assignment: Function return type not convertible to property value type");
				}
			}
			//value assignments
			else if constexpr (std::is_convertible_v<decltype(std::forward<U>(u)), T> ||
							   prop::is_template_specialization_v<U, prop::Property>) {
				return std::function<T()>{nullptr};
			}
			//error
			else {
				static_assert(decltype(u, std::false_type{})::value, "Invalid assignment");
			}
		}()} {
		//move constructor //TODO: change this to std::is_move_assignable_v<base_of<U>, T>
		if constexpr (std::is_same_v<decltype(u), Property<T> &&>) {
			take_explicit_dependents(std::move(u));
			u.sever_implicit_dependents();
		}
	}

	template <class T>
	template <class U>
	Property<T> &Property<T>::operator=(U &&rhs) {
		//function assignments
		if constexpr (detail::is_function_type_v<U>) {
			update_source(std::forward<U>(rhs));
		}
		//value assignments
		else if constexpr (std::is_convertible_v<decltype(std::forward<U>(rhs)), T>) {
			unbind();
			detail::RAII notifier{[this] { write_notify(); }};
			if constexpr (detail::is_equal_comparable_v<T>) {
				T t(std::forward<U>(rhs));
				if (!detail::is_equal(t, value)) {
					value = std::move(t);
				} else {
					notifier.cancel();
				}
			} else {
				value = std::forward<U>(rhs);
			}
		}
		//error
		else {
			static_assert(decltype(rhs, std::false_type{})::value, "Invalid assignment");
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
	template <class U>
	void Property<T>::bind(const Property<U> &other) {
		bind([&other] { return other.get(); });
	}

	template <class T>
	void Property<T>::bind(std::function<T()> f) {
		update_source(std::move(f));
	}

	template <class T>
	void Property<T>::unbind() {
		source = nullptr;
		Property_base::unbind();
	}

	template <class T>
	template <class Functor>
	std::invoke_result_t<Functor, T &> Property<T>::apply(Functor &&f) {
		/* We need to judge if we want to copy value and check if value has changed or avoid the copy and
		 * write_notify even though the value may not have changed. We want to do whichever costs less.
		 * TODO: Figure out a better way to detect cheap to copy types.
		 */
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
			detail::RAII notifier{[this] { write_notify(); }};
			T t = [this, &notifier]() -> T {
				try {
					return source();
				} catch (...) {
					notifier.cancel();
					throw;
				}
			}();
			value = std::move(t);
			updater.early_exit();
			need_update = false;
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
