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
			Signal_binder(std::regular_invocable<const Args &...> auto &&functor, Extra_args &&...extra_args)
				: connection{std::forward<decltype(functor)>(functor)} {}
			Signal_binder(std::regular_invocable<> auto &&functor)
				: connection{[f = std::forward<decltype(functor)>(functor)](const Args &...) { f(); }} {}
			std::function<void(const Args &...)> connection;
		};

		template <class T>
		concept Not_temporary_reference = !std::is_rvalue_reference_v<T>;
	} // namespace detail

	template <detail::Not_temporary_reference... Args>
	class Signal {
		public:
		void emit(const Args &...args) {
			for (auto &connection : connections) {
				connection(args...);
			}
		}
		template <class... Other_args>
			requires(std::convertible_to<Args, Other_args> && ...)
		void connect(Signal<Other_args...> &signal) {
			connections.push_back([&signal](const Args &...args) { signal.emit(args...); });
		}
		void connect(prop::detail::Signal_binder<Args...> binder) {
			connections.push_back(std::move(binder.connection));
		}
		std::size_t number_of_connections() const;
		void disconnect_all() {
			connections.clear();
		}

		private:
		std::vector<std::function<void(const Args &...)>> connections;
	};
} // namespace prop
