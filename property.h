#include <atomic>
#include <functional>
#include <vector>

#include "raii.h"

namespace pro {
	namespace detail {
		struct Property_base {
			virtual void update();
			void read_notify() const;
			static void read_notify(Property_base *base);
			void write_notify();
			void unbind();
			void update_start();
			void update_complete();

			std::vector<Property_base *> dependents;
			std::vector<Property_base *> dependencies;
			bool need_update = false;
			static inline std::vector<Property_base *> *current_binding;
			std::vector<Property_base *> *previous_binding = nullptr;
		};
	} // namespace detail

	template <class T>
	class Property : detail::Property_base {
		public:
		Property() = default;
		Property(T &&t);
		Property(const T &t);
		Property(std::function<T()> f);
		template <class U>
		Property(U &&u);
		template <class U>
		Property &operator=(U &&rhs);
		operator const T &() const;

		private:
		void update_source(std::function<T()> f);
		void update() override final;
		template <class U>
		Property &assign(U &&rhs);
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

		template <class T, class U>
		auto isPropertyCompatible(Property<T> &&p, U &&u) -> decltype(p.assign(u), std::true_type{});
		template <class T, class U>
		std::false_type isPropertyCompatible(...);
		template <class T, class U>
		constexpr bool isPropertyCompatible_v = decltype(isPropertyCompatible(std::declval<T>, std::declval<U>))::value;

	} // namespace detail

	template <class T>
	Property<T>::Property(T &&t)
		: value{std::move(t)} {}

	template <class T>
	Property<T>::Property(const T &t)
		: value{t} {}

	template <class T>
	Property<T>::Property(std::function<T()> f) {
		update_source(std::move(f));
	}

	template <class T>
	template <class U>
	Property<T>::Property(U &&u)
		: Property(std::function<T()>(std::forward<U>(u))) {}

	template <class T>
	template <class U>
	Property<T> &Property<T>::operator=(U &&rhs) {
		//function assignments
		if constexpr (std::is_convertible_v<decltype(std::forward<U>(rhs)), std::function<T()>>) {
			update_source(std::forward<U>(rhs));
		} else if constexpr (std::is_invocable_r_v<T, U>) {
			update_source(std::forward<U>(rhs));
		}
		//value assignments
		else if constexpr (std::is_convertible_v<decltype(std::forward<U>(rhs)), T>) {
			if constexpr (detail::is_equal_comparable_v<T>) {
				T t(std::forward<U>(rhs));
				if (!detail::is_equal(t, value)) {
					value = std::move(t);
					write_notify();
				}
			} else {
				value = std::forward<U>(rhs);
			}
		} else {
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
			if (!detail::is_equal(t, value)) {
				value = std::move(t);
				write_notify();
			}
		} else {
			value = source();
		}
		Property_base::update();
	}

	template <class T>
	Property(T t)
		-> Property<std::conditional_t<std::is_convertible_v<T, std::function<decltype(t())()>>, decltype(t()), T>>;

	//template <class T, class U>
	//auto operator<=>(Property<T> &&lhs, U &&rhs) {
	//	return static_cast<T>(lhs) <=> rhs;
	//}
} // namespace pro
