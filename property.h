#include "raii.h"

#include <functional>
#include <set>

//#define PROPERTY_DEBUG

#ifdef PROPERTY_DEBUG
#include <string>
#endif

namespace pro {
	namespace detail {
		struct Property_base;
		struct Binding_set {
			bool has(Property_base *) const;
			std::size_t count() const;
			void add(Property_base *);
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
			void read_notify() const;
			void read_notify();
			void write_notify();
			void update_start();
			void update_complete();

			Property_base();
			Property_base(const Property_base &) = delete;
			void operator=(const Property_base &) = delete;
			~Property_base();

			Binding_set dependents;
			Binding_set dependencies;
			bool need_update = false;
			static inline Property_base *current_binding;
			Property_base *previous_binding = nullptr;
			Property_base *creation_binding = current_binding;
#ifdef PROPERTY_DEBUG
			std::string name;
#endif

			private:
			void clear_dependencies();
		};
	} // namespace detail

	template <class T>
	class Property : detail::Property_base {
		public:
		Property() = default;
		template <class U>
		Property(U &&u);
		template <class U>
		Property &operator=(U &&rhs);
		operator const T &() const;
#ifdef PROPERTY_DEBUG
		using detail::Property_base::name;
#endif
		private:
		void update_source(std::function<T()> f);
		void update() override final;

		T value;
		std::function<T()> source;
	};

	namespace detail {
		template <class T>
		auto is_equal_comparable(T &&t) -> decltype(t == t, std::true_type{});
		std::false_type is_equal_comparable(...);
		template <class T>
		constexpr bool is_equal_comparable_v = decltype(is_equal_comparable(std::declval<T>))::value;

		template <class T>
		bool is_equal(const T &lhs, const T &rhs) {
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
			//function binding
			if constexpr (detail::is_function_type_v<U>) {
				detail::RAII updater{[this] { update_start(); }, [this] { update_complete(); }};
				return static_cast<T>(u());
			}
			//value assignments
			else if constexpr (std::is_convertible_v<decltype(std::forward<U>(u)), T>) {
				return std::forward<U>(u);
			}
			//error
			else {
				static_assert(decltype(u, std::false_type{})::value, "Invalid assignment");
			}
		}()}
		, source{[&]() -> std::function<T()> {
			//function binding
			if constexpr (detail::is_function_type_v<U>) {
				return static_cast<std::function<T()>>(std::forward<U>(u)());
			}
			//value assignments
			else if constexpr (std::is_convertible_v<decltype(std::forward<U>(u)), T>) {
				return std::function<T()>{nullptr};
			}
			//error
			else {
				static_assert(decltype(u, std::false_type{})::value, "Invalid assignment");
			}
		}()} {}

	template <class T>
	template <class U>
	Property<T> &Property<T>::operator=(U &&rhs) {
		//function assignments
		if constexpr (detail::is_function_type_v<U>) {
			update_source(std::forward<U>(rhs));
		}
		//value assignments
		else if constexpr (std::is_convertible_v<decltype(std::forward<U>(rhs)), T>) {
			if (source) {
				update_start();
				source = nullptr;
				update_complete();
			}
			if constexpr (detail::is_equal_comparable_v<T>) {
				T t(std::forward<U>(rhs));
				if (!detail::is_equal(t, value)) {
					value = std::move(t);
					write_notify();
				}
			} else {
				value = std::forward<U>(rhs);
				write_notify();
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
		read_notify();
		return value;
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
			if (!detail::is_equal(t, value)) {
				value = std::move(t);
				write_notify();
			}
		} else {
			value = source();
			updater.early_exit();
		}
		need_update = false;
	}

	template <class T>
	Property(T &&t) -> Property<detail::inner_type_t<T>>;

	//template <class T, class U>
	//auto operator<=>(Property<T> &&lhs, U &&rhs) {
	//	return static_cast<T>(lhs) <=> rhs;
	//}
} // namespace pro
