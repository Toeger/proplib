#include "widget.h"
#include "widget.privates.h"

void prop::Widget::update() {}

prop::Widget::Widget()
	: privates{std::make_unique<Widget_privates>()} {}

prop::Widget::~Widget() {}
