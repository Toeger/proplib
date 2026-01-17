#include "canvas.h"

#include "prop/platform/platform.h"
#include "prop/ui/widget.h"
#include "prop/utility/font.h"

prop::Canvas::Canvas(Rect<int> rect_, platform::Canvas_context &canvas_context_)
	: rect{std::move(rect_)}
	, canvas_context{&canvas_context_} {}

prop::Canvas::Canvas(platform::Canvas_context &canvas_context_, int width_, int height_)
	: rect{.bottom = height_, .right = width_}
	, canvas_context{&canvas_context_} {}

void prop::Canvas::draw_text(std::string_view text) {
	draw_text(text, font);
}

void prop::Canvas::draw_text(std::string_view text, Font font_) {
	prop::platform::canvas::draw_text(*canvas_context, prop::Rect<>(rect), text, font_);
}

void prop::Canvas::draw_rect(prop::Rect<> rect_, Color color, prop::Pixels width) {
	prop::platform::canvas::draw_rect(*canvas_context, rect_, color, width.amount);
}

prop::Canvas prop::Canvas::sub_canvas_for(const prop::Widget &widget) {
	const auto wrect = static_cast<prop::Rect<int>>(widget.position.get());
	prop::Canvas canvas{{
							.top = rect.top + wrect.top,
							.left = rect.left + wrect.left,
							.bottom = rect.top + wrect.bottom,
							.right = rect.left + wrect.right,
						},
						*canvas_context};
	return canvas;
}
