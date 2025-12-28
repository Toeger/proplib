#pragma once

#include "prop/utility/property_link.h"
#include "prop/utility/type_name.h"
#include "prop/utility/utility.h"

namespace prop {
	template <class T>
	struct Tracking_pointer : private prop::Property_link {
		using prop::Property_link::get_status;
		using prop::Property_link::print_status;

		Tracking_pointer(prop::Property_link *p)
			requires(std::is_base_of_v<prop::Property_link, T>)
			: prop::Property_link{std::vector<prop::Property_link::Property_pointer>{{p, false}}} {}

		constexpr operator T *() const {
			const auto &deps = get_explicit_dependencies();
			assert(deps.size() == 1);
			return dynamic_cast<T *>(deps.front().get_pointer());
		}

		constexpr auto operator->() const {
			return static_cast<T *>(*this);
		}

		private:
		void update() override final {}
		std::string displayed_value() const override {
			const auto &deps = get_explicit_dependencies();
			prop::Property_link *base = deps.empty() ? nullptr : deps.front();
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
	template <class T>
	Tracking_pointer(T *P) -> Tracking_pointer<T>;

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
		requires(std::is_base_of_v<prop::Property_link, T>)
	auto track(T *t) {
		return Tracking_pointer{t};
	}
	template <class T>
		requires(std::is_base_of_v<prop::Property_link, T>)
	auto track(T &t) {
		return Tracking_pointer{&t};
	}
} // namespace prop
