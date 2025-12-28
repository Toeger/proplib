#include "combobox.h"

void prop::swap(Combobox &lhs, Combobox &rhs) noexcept {
	using std::swap;
#define PROP_X(MEMBER) swap(lhs.MEMBER, rhs.MEMBER);
	PROP_X(elements)
	PROP_X(current_element)
#undef PROP_X
		swap(static_cast<prop::Widget &>(lhs), static_cast<prop::Widget &>(rhs));
}
