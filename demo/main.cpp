#include "prop/ui/button.h"
#include "prop/ui/label.h"
#include "prop/ui/vertical_layout.h"
#include "prop/ui/window.h"
#include "prop/utility/widget_helper.h"

#include <iostream>

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
				} widgets;
				[[no_unique_address]] prop::Widget_loader loader{this, widgets};
			} layout;
			prop::Button exit_button{{
				.text = "Exit",
				.callback = [this] { layout.widgets.quit.text = ""; },
			}};
			[[no_unique_address]] prop::Executor set_quit{
				[this] { layout.widgets.quit.callback = exit_button.callback; }};
		} widgets;
		[[no_unique_address]] prop::Widget_loader loader{this, widgets};
	} layout;
	prop::Window w{{
		.title = "Prop Demo",
		.widget = &layout,
	}};
	prop::Window::exec();
}
