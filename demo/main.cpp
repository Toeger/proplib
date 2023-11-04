#include "../proplib/ui/label.h"
#include "../proplib/ui/window.h"

int main(int argc, char *argv[]) {
	prop::Window w{"Prop Demo"};
	auto label = std::make_unique<prop::Label>("Hello world!");
	w.widget = std::move(label);
	w.exec();
}
