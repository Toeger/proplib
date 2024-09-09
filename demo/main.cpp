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
	};
} // namespace prop

int main() {
	struct : prop::Vertical_layout {
		struct {
			prop::Label l1{"Hello world!"};
			prop::Label l2{"Automatic layouting!!!"};
			prop::Label l3{"So cool!!!"};
			prop::Button b1{"Clickable too!!!", [this, current_counter = 0]() mutable {
								std::cout << "Button clicked\n";
								counter.text =
									"Button has been clicked " + std::to_string(++current_counter) + " times!";
							}};
			prop::Label counter{"No clicks yet"};
		} children;
		prop::Widget_loader loader{this, children};
	} layout;
	prop::Window w{"Prop Demo", &layout};
	w.exec();
}
