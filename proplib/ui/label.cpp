#include "label.h"
#include "proplib/platform/platform.h"
#include "proplib/utility/canvas.h"
#include "proplib/utility/utility.h"

#include <boost/pfr/tuple_size.hpp>
#include <cassert>

#define PROP_LABEL_MEMBERS PROP_X(text), PROP_X(font)

prop::Label::Label()
	: Label{Parameters{}} {}

prop::Label::Label(Parameters parameters)
	: prop::Widget{std::move(parameters.widget)}
#define PROP_X(X)                                                                                                      \
	X {                                                                                                                \
		std::move(parameters.X)                                                                                        \
	}
	, PROP_LABEL_MEMBERS
#undef PROP_X
{
	static_assert(boost::pfr::tuple_size_v<prop::Label::Parameters> == 3, "Add missing parameters");
	assert(not prop::Style::default_style.font->name.empty());
	assert(not font->name.empty());
	preferred_size = {
		[](const Label &label) {
			assert(not label.font->name.empty());
			return prop::platform::canvas::text_size(*label.text, label.font);
		},
		self,
	};
}

prop::Label::Label(Label &&other) noexcept {
	swap(*this, other);
}

prop::Label &prop::Label::operator=(Label &&other) noexcept {
	swap(*this, other);
	return *this;
}

prop::Label::~Label() = default;

void prop::Label::draw(Canvas canvas) const {
	canvas.draw_text(text.get(), font);
}

void prop::swap(Label &lhs, Label &rhs) noexcept {
	swap(static_cast<prop::Widget &>(lhs), static_cast<prop::Widget &>(rhs));
#define PROP_X(X) prop::utility::swap(lhs.X, rhs.X)
	(PROP_LABEL_MEMBERS);
#undef PROP_X
}
