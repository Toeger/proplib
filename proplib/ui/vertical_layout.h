#pragma once

#include "../utility/polywrap.h"
#include "widget.h"

#include <memory>
#include <vector>

namespace prop {
	void swap(class Vertical_layout &lhs, class Vertical_layout &rhs);

	class Vertical_layout : public prop::Widget {
		public:
		Vertical_layout();
		template <class... Children>
		Vertical_layout(Children &&...children)
			requires(std::is_convertible_v<decltype(std::forward<Children>(children)), prop::Polywrap<prop::Widget>> &&
					 ...);
		Vertical_layout(Vertical_layout &&other);
		~Vertical_layout();
		Vertical_layout &operator=(Vertical_layout &&other);
		void draw(struct Draw_context context) const override;
		friend void swap(Vertical_layout &lhs, Vertical_layout &rhs);

		prop::Property<std::vector<prop::Polywrap<prop::Widget>>> children;

		private:
		std::unique_ptr<struct Vertical_layout_privates> privates;
	};
} // namespace prop

template <class... Children>
prop::Vertical_layout::Vertical_layout(Children &&...children)
	requires(std::is_convertible_v<decltype(std::forward<Children>(children)), prop::Polywrap<prop::Widget>> && ...)
	: Vertical_layout{} {
	std::vector<prop::Polywrap<prop::Widget>> children_list;
	children_list.reserve(sizeof...(children));
	(children_list.emplace_back(std::forward<Children>(children)), ...);
	this->children = std::move(children_list);
}
