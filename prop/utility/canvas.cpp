#include "canvas.h"

#include "prop/platform/platform.h"
#include "prop/ui/widget.h"
#include "prop/utility/font.h"

prop::Canvas::Canvas(Rect rect_, platform::Canvas_context &canvas_context_)
	: rect{std::move(rect_)}
	, canvas_context{&canvas_context_} {}

prop::Canvas::Canvas(platform::Canvas_context &canvas_context_, int width_, int height_)
	: rect{.bottom = height_, .right = width_}
	, canvas_context{&canvas_context_} {}

void prop::Canvas::draw_text(std::string_view text) {
	draw_text(text, font);
}

void prop::Canvas::draw_text(std::string_view text, Font font_) {
	prop::platform::canvas::draw_text(*canvas_context, rect, text, font_);
}

prop::Canvas prop::Canvas::sub_canvas_for(const prop::Widget &widget) {
	prop::Canvas canvas{{
							.top = rect.top + widget.position->top,
							.left = rect.left + widget.position->left,
							.bottom = rect.top + widget.position->bottom,
							.right = rect.left + widget.position->right,
						},
						*canvas_context};
	return canvas;
}
