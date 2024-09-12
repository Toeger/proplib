#include "../proplib/ui/button.h"
#include "../proplib/ui/label.h"
#include "../proplib/ui/vertical_layout.h"
#include "../proplib/ui/window.h"

#include <iostream>

namespace prop {
	struct Widget_loader {
		template <class Derived_widget, class Children>
		Widget_loader(Derived_widget *dw, Children &children) {
			dw->set_children(children);
		}
		char _[0];
	};

	struct Exec {
		template <class Callable>
		Exec(Callable &&callable) {
			std::forward<Callable>(callable)();
		}
	};
} // namespace prop

int main() {
	struct : prop::Vertical_layout {
		struct {
			struct : prop::Vertical_layout {
				struct {
					prop::Label l1{{
						.text = "Hello world!",
					}};
					prop::Label l2{{
						.text = "Automatic layouting!!!",
					}};
					prop::Label l3{{
						.text = "So cool!!!",
					}};
					prop::Button b1{{
						.text = "Clickable too!!!",
						.callback =
							[this, current_counter = 0]() mutable {
								std::cout << "Button clicked\n";
								counter.text =
									"Button has been clicked " + std::to_string(++current_counter) + " times!";
							},
					}};
					prop::Label counter{{
						.text = "No clicks yet",
					}};
					prop::Button quit{{
						.text = "Reaching out",
					}};
				} children;
				prop::Widget_loader loader{this, children};
			} layout;
			prop::Button exit_button{{
				.text = "Exit",
				.callback = [this] { layout.children.quit.text = ""; },
			}};
			prop::Exec set_quit{[this] { layout.children.quit.callback = exit_button.callback; }};
		} children;
		prop::Widget_loader loader{this, children};
	} layout;
	prop::Window w{{
		.title = "Prop Demo",
		.widget = &layout,
	}};
	w.exec();
}
