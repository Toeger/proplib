#pragma once

#include <cstdint>
#include <type_traits>

namespace prop {
	template <class T>
	struct Required_pointer {
		template <class U>
			requires(std::is_convertible_v<T *, U *>)
		Required_pointer(const Required_pointer<U> &other)
			: data{other.data} {}
		Required_pointer(const T *pointer, bool required)
			requires(alignof(T) >= 2 and !!"The last bit of any T * is assumed to always be 0")
			: data{reinterpret_cast<std::uintptr_t>(pointer) | required} {}
		bool is_required() const {
			return data & 1;
		}
		void set_required(bool required) {
			data = (data & ~std::uintptr_t{1}) | required;
		}
		T *get_pointer() const {
			return reinterpret_cast<T *>(data & ~std::uintptr_t{1});
		}
		T *operator->() const {
			return get_pointer();
		}
		T &operator*() const {
			return *get_pointer();
		}
		operator T *() const {
			return get_pointer();
		}
		Required_pointer &operator=(const T *p) {
			data = reinterpret_cast<std::uintptr_t>(p) | is_required();
			return *this;
		}

		private:
		std::uintptr_t data;
	};
} // namespace prop
