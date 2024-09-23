#include "vertical_layout.h"
#include "proplib/internals/vertical_layout.privates.h"
#include "proplib/internals/widget.privates.h"

#include <cassert>
#ifdef PROPERTY_NAMES
#include <string_view>
#endif

#define PROP_VERTICAL_LAYOUT_PROPERTY_MEMBERS PROP_X(children)

prop::Vertical_layout::Vertical_layout()
	: privates{std::make_unique<Vertical_layout_privates>(this)} {}

prop::Vertical_layout::Vertical_layout(Vertical_layout &&other) {
	swap(*this, other);
}

prop::Vertical_layout::~Vertical_layout() {}

prop::Vertical_layout &prop::Vertical_layout::operator=(Vertical_layout &&other) {
	swap(*this, other);
	return *this;
}

void prop::Vertical_layout::draw(Draw_context context) const {
	for (const auto &child : children.get()) {
		child->draw(context);
	}
}

#ifdef PROPERTY_NAMES
void prop::Vertical_layout::set_name(std::string_view name) {
	name_updater = {[name_ = std::string{name.data(), name.size()}](decltype(children) &children_) {
						std::size_t counter = 0;
						for (auto &child :
							 const_cast<std::remove_cvref_t<decltype(children.get())> &>(children_.get())) {
							child->set_name(name_ + ".children[" + std::to_string(counter++) + "]");
						}
					},
					children};
#define PROP_X(MEMBER) MEMBER.name = std::string{name.data(), name.size()} + "." + #MEMBER;
	PROP_VERTICAL_LAYOUT_PROPERTY_MEMBERS
	PROP_X(privates->layout_updater)
#undef PROP_X
	prop::Widget::set_name(std::move(name));
}

prop::Vertical_layout::Vertical_layout(std::string_view name)
	: privates{std::make_unique<Vertical_layout_privates>(this)} {
	set_name(name);
}
#endif

void prop::swap(Vertical_layout &lhs, Vertical_layout &rhs) {
	using std::swap;
#define PROP_X(MEMBER) swap(lhs.MEMBER, rhs.MEMBER);
	PROP_VERTICAL_LAYOUT_PROPERTY_MEMBERS
	PROP_X(privates)
#undef PROP_X
	swap(static_cast<prop::Widget &>(lhs), static_cast<prop::Widget &>(rhs));
}
