#include "../proplib/ui/label.h"
#include "../proplib/ui/window.h"

int main() {
	prop::Window w{"Prop Demo"};
	w.widget = prop::Label{"Hello world!"};
	w.exec();
}
