#pragma once

#include "property.h"

#include <concepts>
#include <functional>
#include <type_traits>
#include <vector>

namespace prop {
	namespace detail {
		template <class... Args>
		struct Signal_binder {
			template <class... Extra_args>
			Signal_binder(std::regular_invocable<const Args &...> auto &&functor, Extra_args &&...extra_args);
			std::function<void(const Args &...)> connection;
		};

		template <class T>
		concept Not_temporary_reference = !std::is_rvalue_reference_v<T>;
	} // namespace detail

	template <detail::Not_temporary_reference... Args>
	class Signal {
		public:
		void emit(const Args &...args);
		template <class... Other_args>
			requires(std::convertible_to<Args, Other_args> && ...)
		void connect(const Signal<Other_args...> &signal);
		void connect(detail::Signal_binder<Args...> binder);
		std::size_t number_of_connections() const;
		void disconnect_all();

		private:
		std::vector<std::function<void(const Args &...)>> connections;
	};
} // namespace prop

//implementation
namespace prop {
	inline void temptest() {
		prop::Signal<int, double, std::vector<int> &> s;
	}

	template <class... Args>
	template <class... Extra_args>
	detail::Signal_binder<Args...>::Signal_binder(std::regular_invocable<const Args &...> auto &&functor,
												  Extra_args &&...extra_args)
		: connection{std::forward<decltype(functor)>(functor)} {}

	template <detail::Not_temporary_reference... Args>
	void Signal<Args...>::emit(const Args &...args) {
		for (auto &connection : connections) {
			connection(args...);
		}
	}

	template <detail::Not_temporary_reference... Args>
	void Signal<Args...>::connect(detail::Signal_binder<Args...> binder) {
		connections.push_back(std::move(binder.connection));
	}

	template <detail::Not_temporary_reference... Args>
	void Signal<Args...>::disconnect_all() {
		connections.clear();
	}
} // namespace prop
