#include "vertical_layout.h"
#include "../internals/vertical_layout.privates.h"
#include "../internals/widget.privates.h"

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

void prop::swap(Vertical_layout &lhs, Vertical_layout &rhs) {
	using std::swap;
#define PROP_X(MEMBER) swap(lhs.MEMBER, rhs.MEMBER);
	PROP_X(children)
	PROP_X(privates)
#undef PROP_X
		swap(static_cast<prop::Widget &>(lhs), static_cast<prop::Widget &>(rhs));
}
