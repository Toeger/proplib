#include "vertical_layout.h"
#include "prop/utility/canvas.h"
#include "prop/utility/dependency_tracer.h"
#include "prop/utility/tracking_pointer.h"

#include <cassert>
#ifdef PROPERTY_NAMES
#include <string_view>
#endif

#ifndef PROP_SCREEN_UNIT_PRECISION
#define PROP_SCREEN_UNIT_PRECISION std::float_t
#endif

#define PROP_VERTICAL_LAYOUT_PROPERTY_MEMBERS                                                                          \
	PROP_X(name_updater), PROP_X(children), PROP_X(alignment), PROP_X(child_positioner)

prop::Vertical_layout::Vertical_layout()
	: child_positioner{[self_pointer = prop::track(this)] mutable {
		//return;
		self_pointer->children.apply([&self = *self_pointer](std::vector<prop::Polywrap<prop::Widget>> &children_) {
			if (children_.empty()) {
				self.min_size = prop::Size<>{0, 0};
				self.max_size = prop::Size<>::max;
				self.preferred_size = prop::Size<>{0, 0};
				return;
			}
			PROP_SCREEN_UNIT_PRECISION min_width = 0;
			PROP_SCREEN_UNIT_PRECISION min_height = 0;
			PROP_SCREEN_UNIT_PRECISION max_width = 0;
			PROP_SCREEN_UNIT_PRECISION max_height = 0;
			PROP_SCREEN_UNIT_PRECISION ypos = 0;
			const PROP_SCREEN_UNIT_PRECISION width = self.position->width();
			for (auto &child : children_) {
				//TODO: Don't just go with minimum size, instead take actual size into account and spread leftover space across widgets
				const auto &min_size = child->get_min_size().get();
				const auto &max_size = child->get_max_size().get();
				min_width = std::max(min_width, min_size.width);
				min_height += min_size.height;
				max_width = std::min(max_width, max_size.width);
				max_height += max_size.height;
				child->position = Rect<>{
					.top = ypos,
					.left = 0,
					.bottom = ypos + min_size.height,
					.right = width,
				};
				ypos += min_size.height;
			}
			self.min_size = prop::Size{min_width, min_height};
			self.max_size = prop::Size{max_width, max_height};
		});
	}}
	, name_updater{
		  [this](decltype(children) &children_) {
			  std::size_t counter = 0;
			  for (auto &child : children_.get()) {
				  child->set_name(custom_name + ".children[" + std::to_string(counter++) + "]");
			  }
		  },
		  children,
	  } {
	assert(child_positioner.is_dependent_on(children));
	custom_name = std::format("<{}>", prop::type_name<std::remove_cvref_t<decltype(*this)>>());
}

prop::Vertical_layout::Vertical_layout(Vertical_layout &&other) noexcept {
	swap(*this, other);
}

prop::Vertical_layout::~Vertical_layout() {}

prop::Vertical_layout &prop::Vertical_layout::operator=(Vertical_layout &&other) noexcept {
	swap(*this, other);
	return *this;
}

void prop::Vertical_layout::draw(Canvas context) const {
	for (const auto &child : children.get()) {
		child->draw(context.sub_canvas_for(*child.get()));
	}
}

#ifdef PROPERTY_NAMES
void prop::Vertical_layout::set_name(std::string_view name) {
#define PROP_X(MEMBER) MEMBER.custom_name = std::string{name} + "." + #MEMBER
	(PROP_VERTICAL_LAYOUT_PROPERTY_MEMBERS);
#undef PROP_X
	prop::Widget::set_name(std::move(name));
	name_updater.update();
}

prop::Vertical_layout::Vertical_layout(std::string_view name) {
	set_name(name);
}
#endif

void prop::swap(Vertical_layout &lhs, Vertical_layout &rhs) {
	using std::swap;
#define PROP_X(MEMBER) swap(lhs.MEMBER, rhs.MEMBER)
	(PROP_VERTICAL_LAYOUT_PROPERTY_MEMBERS);
#undef PROP_X
	swap(static_cast<prop::Widget &>(lhs), static_cast<prop::Widget &>(rhs));
}

void prop::Vertical_layout::trace(Dependency_tracer &dependency_tracer) const {
	prop::Dependency_tracer::Make_current _{*this, dependency_tracer};
#define PROP_X(X) PROP_TRACE(dependency_tracer, X)
	(PROP_VERTICAL_LAYOUT_PROPERTY_MEMBERS);
#undef PROP_X
	for (std::size_t i = 0; i < std::size(children.get()); i++) {
		auto &child = *children[i];
		dependency_tracer.trace(child);
	}
	prop::Widget::trace(dependency_tracer);
}
