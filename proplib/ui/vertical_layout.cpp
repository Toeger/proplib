#include "vertical_layout.h"
#include "proplib/utility/canvas.h"
#include "proplib/utility/dependency_tracer.h"

#include <cassert>
#ifdef PROPERTY_NAMES
#include <string_view>
#endif

#define PROP_VERTICAL_LAYOUT_PROPERTY_MEMBERS                                                                          \
	PROP_X(name_updater), PROP_X(children), PROP_X(alignment), PROP_X(child_positioners)

prop::Vertical_layout::Vertical_layout()
	: child_positioners{[self_ = prop::track(this)] mutable {
		//return;
		self_->children.apply([self = std::move(self_)](std::vector<prop::Polywrap<prop::Widget>> &children_) {
			if (children_.empty()) {
				self->min_size = prop::Size{0, 0};
				self->max_size = prop::Size::max;
				self->preferred_size = prop::Size{0, 0};
				return;
			}
			int min_width = 0;
			int min_height = 0;
			int max_width = 0;
			int max_height = 0;
			int ypos = 0;
			const int width = self->position->width();
			for (auto &child : children_) {
				//TODO: Don't just go with minimum size, instead take actual size into account and spread leftover space across widgets
				const auto &min_size = child->get_min_size().get();
				const auto &max_size = child->get_max_size().get();
				min_width = std::max(min_width, min_size.width);
				min_height += min_size.height;
				max_width = std::min(max_width, max_size.width);
				max_height += max_size.height;
				child->position = Rect{
					.top = ypos,
					.left = 0,
					.bottom = ypos + min_size.height,
					.right = width,
				};
				ypos += min_size.height;
			}
			self->min_size = prop::Size{min_width, min_height};
			self->max_size = prop::Size{max_width, max_height};
		});
	}} {
	//assert(child_positioners.is_dependent_on(children));
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
	name_updater = {
		[name_ = std::string{name.data(), name.size()}](decltype(children) &children_) {
			std::size_t counter;
			counter = 0;
			for (auto &child : const_cast<std::remove_cvref_t<decltype(children.get())> &>(children_.get())) {
				child->set_name(name_ + ".children[" + std::to_string(counter++) + "]");
			}
		},
		children,
	};
#define PROP_X(MEMBER) MEMBER.custom_name = std::string{name.data(), name.size()} + "." + #MEMBER
	(PROP_VERTICAL_LAYOUT_PROPERTY_MEMBERS);
#undef PROP_X
	prop::Widget::set_name(std::move(name));
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
	prop::Dependency_tracer::Make_current _{this, dependency_tracer};
#define PROP_X(X) PROP_TRACE(dependency_tracer, X)
	(PROP_VERTICAL_LAYOUT_PROPERTY_MEMBERS);
#undef PROP_X
	for (auto &child : children.get()) {
		dependency_tracer.trace_child(*child);
	}
	dependency_tracer.trace_base(static_cast<const prop::Widget *>(this));
}
