#include "widget.h"
#include "proplib/internals/widget.privates.h"

#include <boost/pfr/tuple_size.hpp>
#ifdef PROPERTY_NAMES
#include <string_view>
#endif

#define PROP_WIDGET_PROPERTY_MEMBERS                                                                                   \
	PROP_X(x) PROP_X(y) PROP_X(width) PROP_X(height) PROP_X(preferred_width) PROP_X(preferred_height) PROP_X(visible)

prop::Widget::Widget()
	: Widget{Parameters{}} {
#ifdef PROPERTY_NAMES
	set_name("<Widget>");
#endif
}

prop::Widget::Widget(Parameters &&parameter)
	: x{std::move(parameter.x)}
	, y{std::move(parameter.y)}
	, width{std::move(parameter.width)}
	, height{std::move(parameter.height)}
	, preferred_width{std::move(parameter.preferred_width)}
	, preferred_height{std::move(parameter.preferred_height)}
	, visible{std::move(parameter.visible)}
	, privates{std::make_unique<Widget_privates>()} {
	static_assert(boost::pfr::tuple_size_v<prop::Widget::Parameters> == 7, "Add missing parameters");
}

prop::Widget::Widget(Widget &&other) {
	swap(*this, other);
}

prop::Widget &prop::Widget::operator=(Widget &&other) {
	swap(*this, other);
	return *this;
}

prop::Widget::~Widget() = default;

void prop::Widget::draw(Draw_context) const {}

#ifdef PROPERTY_NAMES
void prop::Widget::set_name(std::string_view name){
#define PROP_X(MEMBER) MEMBER.name = std::string{name.data(), name.size()} + "." #MEMBER;
	PROP_WIDGET_PROPERTY_MEMBERS
#undef PROP_X
}

prop::Widget::Widget(std::string_view name)
	: privates{std::make_unique<Widget_privates>()} {
	set_name(name);
}
#endif

void prop::swap(Widget &lhs, Widget &rhs) {
	using std::swap;
#define PROP_X(MEMBER) swap(lhs.MEMBER, rhs.MEMBER);
	PROP_WIDGET_PROPERTY_MEMBERS
	PROP_X(privates)
#undef PROP_X
}
