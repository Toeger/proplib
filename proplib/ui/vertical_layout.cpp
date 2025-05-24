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
	: child_positioners{[self = selfie()](std::vector<Child_positioner> &positioners) {
		//return prop::Updater_result::unchanged;
		auto changed = prop::Updater_result::unchanged;
		positioners.resize(self->children->size());
		for (std::size_t i = 0; i < positioners.size(); i++) {
			auto child = self->children[i].get();
			auto &positioner = positioners[i];
			if (positioner.widget != child or positioner.index != i) {
				changed = prop::Updater_result::changed;
				positioner.widget = child;
				positioner.index = i;
				positioner.position = [self_ = self->selfie(), i] {
					const auto preferred_size =
						self_->children->size() < i ? self_->children[i]->get_preferred_size().get() : prop::Size{};
					auto &&prev_pos = i ? self_->child_positioners[i - 1].position.get() :
										  Rect{
											  .top = self_->position->top,
											  .left = self_->position->left,
											  .bottom = self_->position->top,
											  .right = self_->position->right,
										  };
					return Rect{
						.top = prev_pos.bottom,
						.left = self_->position->left,
						.bottom = prev_pos.bottom + preferred_size.height,
						.right = self_->position->right,
					};
				};
			}
		}
		return changed;
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
		[name_ = std::string{name.data(), name.size()}](
			decltype(children) &children_, [[maybe_unused]] decltype(child_positioners) &child_positioners_) {
			std::size_t counter;
			counter = 0;
			for (auto &child : const_cast<std::remove_cvref_t<decltype(children.get())> &>(children_.get())) {
				child->set_name(name_ + ".children[" + std::to_string(counter++) + "]");
			}
#ifdef PROPERTY_NAMES
			counter = 0;
			for (auto &child_pos :
				 const_cast<std::remove_cvref_t<decltype(child_positioners_.get())> &>(child_positioners_.get())) {
				child_pos.position.custom_name = name_ + ".children[" + std::to_string(counter++) + "]";
			}
#endif
		},
		children,
		child_positioners,
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
