#pragma once

#include "proplib/utility/property_details.h"
#include "proplib/utility/utility.h"

namespace prop {
	template <class T>
	struct Tracking_pointer : private prop::detail::Property_base {
		using prop::detail::Property_base::get_status;
		using prop::detail::Property_base::print_status;
		Tracking_pointer(T *p)
			requires(std::is_base_of_v<prop::detail::Property_base, T>)
			: prop::detail::Property_base{
				  std::vector<prop::detail::Property_link>{{(prop::detail::Property_base *)p, false}}} {}
		constexpr operator T *() const {
			const auto &deps = get_explicit_dependencies();
			if (deps.empty()) {
				return nullptr;
			}
			prop::detail::Property_base *b = deps.front();
			if constexpr (std::is_convertible_v<T *, prop::detail::Property_base *>) {
				return dynamic_cast<T *>(b);
			} else {
				return (T *)b;
			}
		}

		private:
		void update() override final {}
		std::string displayed_value() const override {
			const auto &deps = get_explicit_dependencies();
			prop::detail::Property_base *base = deps.empty() ? nullptr : deps.front();
			auto dynamic = static_cast<T *>(*this);
			if (reinterpret_cast<std::uintptr_t>(base) != reinterpret_cast<std::uintptr_t>(dynamic)) {
				return prop::to_string(base) + "(" + prop::to_string(dynamic) + ")";
			}
			return prop::to_string(dynamic);
		}
		std::string_view type() const override {
			return prop::type_name<Tracking_pointer<T>>();
		}
		std::string value_string() const override {
			return displayed_value();
		}
		bool has_source() const override {
			return static_cast<T *>(*this) != nullptr;
		}
	};

#define PROP_COMP(OP)                                                                                                  \
	template <class T>                                                                                                 \
	constexpr auto operator OP(const prop::Tracking_pointer<T> &tp, T *p) {                                            \
		return static_cast<T *>(tp) OP p;                                                                              \
	}                                                                                                                  \
	template <class T>                                                                                                 \
	constexpr auto operator OP(T *p, const prop::Tracking_pointer<T> &tp) {                                            \
		return p OP static_cast<T *>(tp);                                                                              \
	}                                                                                                                  \
	template <class T>                                                                                                 \
	constexpr auto operator OP(const prop::Tracking_pointer<T> &tp1, const prop::Tracking_pointer<T> &tp2) {           \
		return static_cast<T *>(tp1) OP static_cast<T *>(tp2);                                                         \
	}
	PROP_COMP(==);
	PROP_COMP(!=);
	PROP_COMP(<);
	PROP_COMP(>);
	PROP_COMP(<=);
	PROP_COMP(>=);
	PROP_COMP(<=>);
#undef PROP_COMP

	template <class T>
		requires(::std::is_base_of_v<prop::detail::Property_base, T>)
	auto track(T *t) {
		return Tracking_pointer{t};
	}
} // namespace prop
