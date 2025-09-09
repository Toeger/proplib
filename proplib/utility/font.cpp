#include "font.h"
#include "proplib/utility/style.h"

#define PROP_MEMBERS                                                                                                   \
	PROP_X(color), PROP_X(name), PROP_X(alignment), PROP_X(size), PROP_X(bold), PROP_X(italic), PROP_X(strikeout),     \
		PROP_X(underline)

prop::Font::Font()
#define PROP_X(X)                                                                                                      \
	X {                                                                                                                \
		prop::Style::default_style.font->X                                                                             \
	}
	: PROP_MEMBERS
#undef PROP_X
{
}

prop::Font::Font(Attributes attributes_)
#define PROP_X(X)                                                                                                      \
	X {                                                                                                                \
		attributes_.X ? std::move(attributes_.X.value()) : prop::Style::default_style.font->X                          \
	}
	: PROP_MEMBERS
#undef PROP_X
{
}

void prop::Font::set(Attributes attributes_) {
#define PROP_X(X)                                                                                                      \
	[&] {                                                                                                              \
		if (attributes_.X) {                                                                                           \
			X = std::move(attributes_.X.value());                                                                      \
		}                                                                                                              \
	}()
	(PROP_MEMBERS);
#undef PROP_X
}

prop::Font prop::Font::with(Attributes attributes_) {
	Font font{*this};
	font.set(std::move(attributes_));
	return font;
}
