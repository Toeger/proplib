#include "../proplib/ui/label.h"
#include "../proplib/ui/window.h"

int main(int argc, char *argv[]) {
	prop::Window w{"Prop Demo"};
	auto label = std::make_unique<prop::Label>("Hello world!");
	label->x = 100;
	label->y = 200;
	w.widget = std::move(label);
	w.exec();
}
