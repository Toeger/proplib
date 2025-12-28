#include "window.h"
#include "prop/platform/platform.h"
#include "prop/utility/canvas.h"

#include <SFML/Graphics.hpp>

//static auto get_widget_updater(prop::Window &window) {
//	return [&window] {
//		if (window.widget.get()) {
//			window.widget.apply([&window](prop::Polywrap<prop::Widget> &widget) {
//				widget->x = widget->y = 0;
//				widget->width = {[](int width) { return width; }, window.width};
//				widget->height = {[](int height) { return height; }, window.height};
//			});
//		}
//	};
//}

#define PROP_MEMBERS PROP_X(size), PROP_X(title), PROP_X(widget)

prop::Window::Window()
	: Window{Parameters{}} {}

prop::Window::Window(Parameters &&parameters)
#define PROP_X(X)                                                                                                      \
	X {                                                                                                                \
		std::move(parameters.X)                                                                                        \
	}
	: PROP_MEMBERS
#undef PROP_X
	, platform_window_{prop::platform::Window::create({
		  .width = size->width,
		  .height = size->height,
		  .title = title.get(),
		  .window = this,
	  })} {
	//auto widget_updater = get_widget_updater(*this);
	//widget_updater();
}

prop::Window::Window(Window &&other) noexcept
#define PROP_X(X)                                                                                                      \
	X {                                                                                                                \
		std::move(other.X)                                                                                             \
	}
	: PROP_MEMBERS
#undef PROP_X
	, platform_window_{std::move(other.platform_window_)} {
	platform_window_->window = this;
}

prop::Window &prop::Window::operator=(prop::Window &&other) noexcept {
#define PROP_X(X) void(X = std::move(other.X))
	(PROP_MEMBERS);
#undef PROP_X
	platform_window_ = std::move(other.platform_window_);
	return *this;
}

prop::platform::Window &prop::Window::platform_window() const {
	return *platform_window_;
}

void prop::Window::pump() {
	prop::platform::Window::pump();
}

void prop::Window::exec() {
	prop::platform::Window::exec();
}
