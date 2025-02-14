#include "vertical_layout.h"
#include "proplib/utility/canvas.h"

#include <cassert>
#ifdef PROPERTY_NAMES
#include <string_view>
#endif

#define PROP_VERTICAL_LAYOUT_PROPERTY_MEMBERS PROP_X(children) PROP_X(alignment) PROP_X(child_positioners)

prop::Vertical_layout::Vertical_layout()
	: child_positioners{[this](std::vector<Child_positioner> &positioners) {
		auto changed = prop::Value::unchanged;
		positioners.resize(children->size());
		for (std::size_t i = 0; i < positioners.size(); i++) {
			auto child = children[i].get();
			auto &positioner = positioners[i];
			if (positioner.widget != child or positioner.index != i) {
				changed = prop::Value::changed;
				positioner.widget = child;
				positioner.index = i;
				positioner.position.sever();
				positioner.position = [this, i] {
					const auto preferred_size =
						children->size() < i ? children[i]->get_preferred_size().get() : prop::Size{};
					auto &&prev_pos = i ? child_positioners[i - 1].position.get() :
										  Rect{
											  .top = rect->top,
											  .left = rect->left,
											  .bottom = rect->top,
											  .right = rect->right,
										  };
					return Rect{
						.top = prev_pos.bottom,
						.left = rect->left,
						.bottom = prev_pos.bottom + preferred_size.height,
						.right = rect->right,
					};
				};
			}
		}
		return changed;
	}} {
	assert(child_positioners.is_dependent_on(children));
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
	name_updater = {[name_ = std::string{name.data(), name.size()}](decltype(children) &children_) {
						std::size_t counter = 0;
						for (auto &child :
							 const_cast<std::remove_cvref_t<decltype(children.get())> &>(children_.get())) {
							child->set_name(name_ + ".children[" + std::to_string(counter++) + "]");
						}
					},
					children};
#define PROP_X(MEMBER) MEMBER.custom_name = std::string{name.data(), name.size()} + "." + #MEMBER;
	PROP_VERTICAL_LAYOUT_PROPERTY_MEMBERS
#undef PROP_X
	prop::Widget::set_name(std::move(name));
}

prop::Vertical_layout::Vertical_layout(std::string_view name) {
	set_name(name);
}
#endif

void prop::swap(Vertical_layout &lhs, Vertical_layout &rhs) {
	using std::swap;
#define PROP_X(MEMBER) swap(lhs.MEMBER, rhs.MEMBER);
	PROP_VERTICAL_LAYOUT_PROPERTY_MEMBERS
#undef PROP_X
	swap(static_cast<prop::Widget &>(lhs), static_cast<prop::Widget &>(rhs));
}
