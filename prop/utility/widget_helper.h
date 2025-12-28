#pragma once

#include <utility>

namespace prop {
	namespace detail {
		struct Empty {
#ifdef PROP_EMPTY_CLASS_OPTIMIZATION
			char _[0];
#endif
		};
	} // namespace detail

	struct Widget_loader : private prop::detail::Empty {
		template <class Derived_widget, class Children>
		Widget_loader(Derived_widget *dw, Children &&children) {
			dw->set_children(std::forward<Children>(children));
		}
	};

	struct Executor : private prop::detail::Empty {
		template <class Callable>
		Executor(Callable &&callable) {
			std::forward<Callable>(callable)();
		}
	};
} // namespace prop
