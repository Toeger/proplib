#include "widget.h"
#include "widget.privates.h"

void prop::Widget::update() {}

prop::Widget::Widget()
	: privates{std::make_unique<Widget_privates>()} {}

prop::Widget::Widget(Widget &&other) {
	swap(*this, other);
}

prop::Widget &prop::Widget::operator=(Widget &&other) {
	swap(*this, other);
	return *this;
}

prop::Widget::~Widget() {}

void prop::swap(Widget &lhs, Widget &rhs) {
	using std::swap;
	swap(lhs.x, rhs.x);
	swap(lhs.y, rhs.y);
	swap(lhs.width, rhs.width);
	swap(lhs.height, rhs.height);
	swap(lhs.privates, rhs.privates);
}
