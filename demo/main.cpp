#include "../proplib/ui/label.h"
#include "../proplib/ui/vertical_layout.h"
#include "../proplib/ui/window.h"

int main() {
	prop::Window w{"Prop Demo"};
	w.widget = prop::Vertical_layout{
		prop::Label{"Hello world!"},
		prop::Label{"Automatic layouting!!!"},
		prop::Label{"So cool!!!"},
	};
	w.exec();
}
