#pragma once

#include "type_name.h"
#include <type_traits>
#include <utility>

namespace prop {
	//store and manipulate types
	template <class... Ts>
	struct Type_list {
		private:
		//helper to get type at index
		template <int index, class... Args>
		struct Get_at;
		template <class T, class... Args>
		struct Get_at<0, T, Args...> {
			using Type = T;
		};
		template <int index>
		struct Get_at<index> {};
		template <int index, class T, class... Args>
		struct Get_at<index, T, Args...> {
			using Type = typename Get_at<index - 1, Args...>::Type;
		};
		//helper to remove a type
		template <class T, class... Args>
		struct Remove;
		template <class T>
		struct Remove<T> {
			using types = Type_list<>;
		};
		template <class T, class Head, class... Tail>
		struct Remove<T, Head, Tail...> {
			using types = std::conditional_t<std::is_same_v<T, Head>, typename Remove<T, Tail...>::types,
											 typename Remove<T, Tail...>::types::template prepend<Head>>;
		};
		//helper for contains
		template <class T, class... Args>
		struct Contains {
			static constexpr bool value = false;
		};
		template <class T, class First, class... Args>
		struct Contains<T, First, Args...> {
			static constexpr bool value = std::is_same_v<T, First> or Contains<T, Args...>::value;
		};
		template <class... Args>
		static auto concat(Type_list<Args...>) -> Type_list<Ts..., Args...>;

		public:
		template <class... T>
		using append = Type_list<Ts..., T...>;
		template <class... T>
		using prepend = Type_list<T..., Ts...>;
		template <template <typename...> class F>
		using apply = Type_list<F<Ts>...>;
		template <template <typename...> class T>
		using instantiate = T<Ts...>;
		template <int index>
		using at = typename Get_at < index<0 ? sizeof...(Ts) + index : index, Ts...>::Type;
		template <class T>
		using concatenate = decltype(concat(T{}));
		template <class T>
		using remove = typename Remove<T, Ts...>::types;
		template <class T>
		constexpr static bool contains_v = Contains<T, Ts...>::value;
		constexpr static std::size_t size = sizeof...(Ts);
		using index_sequence = std::index_sequence_for<Ts...>;
		static constexpr std::string_view type_names = [] {
			std::string_view name = prop::type_name<prop::Type_list<Ts...>>();
			name.remove_suffix(1);
			name.remove_prefix(sizeof("prop::type_name"));
			return name;
		}();

		//TODO: sort, remove_if
	};
	namespace detail {
		template <template <typename...> class T, class... Args>
		auto adopt_from(T<Args...> &&) -> Type_list<Args...>;
	}
	template <class T>
	using adopt_from = decltype(prop::detail::adopt_from(std::declval<T>()));
} // namespace prop
