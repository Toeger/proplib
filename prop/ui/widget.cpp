#include "widget.h"
#include "prop/utility/canvas.h"
#include "prop/utility/dependency_tracer.h"
#include "prop/utility/utility.h"

#include <boost/pfr/tuple_size.hpp>
#ifdef PROPERTY_NAMES
#include <string_view>
#endif

#define PROP_WIDGET_PROPERTY_MEMBERS                                                                                   \
	PROP_X(position), PROP_X(visible), PROP_X(min_size), PROP_X(max_size), PROP_X(preferred_size)

prop::Widget::Widget()
	: Widget{Parameters{}} {
#ifdef PROPERTY_NAMES
	set_name("<Widget>");
#endif
}

prop::Widget::Widget(Parameters &&parameter)
#define PROP_X(X)                                                                                                      \
	X {                                                                                                                \
		std::move(parameter.X)                                                                                         \
	}
	: PROP_WIDGET_PROPERTY_MEMBERS
#undef PROP_X
{
	static_assert(boost::pfr::tuple_size_v<prop::Widget::Parameters> == 5, "Add missing parameters");
}

prop::Widget::Widget(Widget &&other) noexcept {
	swap(*this, other);
}

prop::Widget &prop::Widget::operator=(Widget &&other) noexcept {
	swap(*this, other);
	return *this;
}

prop::Widget::~Widget() = default;

void prop::Widget::draw(prop::Canvas) const {}

#ifdef PROPERTY_NAMES
void prop::Widget::set_name(std::string_view name) {
#define PROP_X(MEMBER) MEMBER.custom_name = std::string{name} + "." #MEMBER
	(PROP_WIDGET_PROPERTY_MEMBERS);
#undef PROP_X
}

void prop::Widget::trace(Dependency_tracer &dependency_tracer) const {
	Dependency_tracer::Make_current _{*this, dependency_tracer};
#define PROP_X(X) PROP_TRACE(dependency_tracer, X)
	(PROP_WIDGET_PROPERTY_MEMBERS);
#undef PROP_X
}

prop::Widget::Widget(std::string_view name) {
	set_name(name);
}
#endif

void prop::swap(Widget &lhs, Widget &rhs) {
#define PROP_X(MEMBER) prop::utility::swap(lhs.MEMBER, rhs.MEMBER)
	(PROP_WIDGET_PROPERTY_MEMBERS);
#undef PROP_X
}
