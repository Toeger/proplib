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

void prop::swap(prop::Widget &lhs, prop::Widget &rhs) {
	using std::swap;
#define PROP_MEMBERS                                                                                                   \
	PROP_X(x) PROP_X(y) PROP_X(width) PROP_X(height) PROP_X(privates) PROP_X(preferred_width) PROP_X(preferred_height)
#define PROP_X(MEMBER) swap(lhs.MEMBER, rhs.MEMBER);
	PROP_MEMBERS
#undef PROP_X
#undef PROP_MEMBERS
}
