#include "../proplib/ui/button.h"
#include "../proplib/ui/label.h"
#include "../proplib/ui/vertical_layout.h"
#include "../proplib/ui/window.h"

#include <iostream>

int main() {
	prop::Window w{"Prop Demo"};
	w.widget = prop::Vertical_layout{
		prop::Label{"Hello world!"},
		prop::Label{"Automatic layouting!!!"},
		prop::Label{"So cool!!!"},
		prop::Button{"Clickable too!!!", []{std::cout << "Button clicked\n";}},
	};
	w.exec();
}
