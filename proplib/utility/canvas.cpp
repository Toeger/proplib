#include "canvas.h"

#include "proplib/platform/platform.h"
#include "proplib/ui/widget.h"
#include "proplib/utility/font.h"

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
							.top = rect.top + widget.rect->top,
							.left = rect.left + widget.rect->left,
							.bottom = rect.top + widget.rect->bottom,
							.right = rect.left + widget.rect->right,
						},
						*canvas_context};
	return canvas;
}
