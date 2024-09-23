#pragma once

#include "proplib/ui/widget.h"
#include "proplib/utility/polywrap.h"

#include <boost/pfr/core.hpp>
#include <memory>
#include <vector>
#ifdef PROPERTY_NAMES
#include <string_view>
#endif

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
		~Vertical_layout() override;
		Vertical_layout &operator=(Vertical_layout &&other);
		void draw(struct Draw_context context) const override;
		friend void swap(Vertical_layout &lhs, Vertical_layout &rhs);
		template <class... Args>
		void set_children(Args &&...args);
#ifdef PROPERTY_NAMES
		void set_name(std::string_view name) override;
		prop::Property<void> name_updater;
		Vertical_layout(std::string_view name);
		template <class... Children>
		Vertical_layout(std::string_view name, Children &&...children)
			requires(std::is_convertible_v<decltype(std::forward<Children>(children)), prop::Polywrap<prop::Widget>> &&
					 ...)
			: Vertical_layout{children...} {
			set_name(name);
		}
#endif

		prop::Property<std::vector<prop::Polywrap<prop::Widget>>> children;

		private:
		template <class... Args, std::size_t... indexes>
		static void add_children(std::vector<prop::Polywrap<prop::Widget>> &container, std::index_sequence<indexes...>,
								 Args &&...args);

		std::unique_ptr<struct Vertical_layout_privates> privates;
	};
} // namespace prop

//implementation
template <class... Children>
prop::Vertical_layout::Vertical_layout(Children &&...children_)
	requires(std::is_convertible_v<decltype(std::forward<Children>(children_)), prop::Polywrap<prop::Widget>> && ...)
	: Vertical_layout{} {
	std::vector<prop::Polywrap<prop::Widget>> children_list;
	children_list.reserve(sizeof...(children_));
	(children_list.emplace_back(std::forward<Children>(children_)), ...);
	this->children = std::move(children_list);
#ifdef PROPERTY_NAMES
	set_name("Vertical_layout");
#endif
}

namespace prop {
	namespace detail {
		void add_child(std::vector<prop::Polywrap<prop::Widget>> &container,
					   prop::detail::Settable<prop::Polywrap<prop::Widget>> auto &&child) {
			container.emplace_back(std::forward<decltype(child)>(child));
		}
		template <class Non_widget_member>
		void add_child(std::vector<prop::Polywrap<prop::Widget>> &, Non_widget_member &&) {}
		template <class Aggregate, std::size_t... indexes>
		void add_children(std::vector<prop::Polywrap<prop::Widget>> &container, std::index_sequence<indexes...>,
						  Aggregate &&aggregate) {
			(add_child(container, &boost::pfr::get<indexes>(aggregate)), ...);
		}
		template <class Child>
			requires(std::is_aggregate_v<std::remove_reference_t<Child>> &&
					 not prop::detail::Settable<Polywrap<prop::Widget>, Child>)
		void add_child(std::vector<prop::Polywrap<prop::Widget>> &container, Child &&child) {
			add_children(container,
						 std::make_index_sequence<boost::pfr::tuple_size_v<std::remove_reference_t<Child>>>(),
						 std::forward<Child>(child));
		}
	} // namespace detail
} // namespace prop

template <class... Children>
void prop::Vertical_layout::set_children(Children &&...children_) {
	std::vector<prop::Polywrap<prop::Widget>> container;
	add_children(container, std::index_sequence_for<Children...>(), std::forward<Children>(children_)...);
	this->children = std::move(container);
}

template <class... Children, std::size_t... indexes>
void prop::Vertical_layout::add_children(std::vector<prop::Polywrap<prop::Widget>> &container,
										 std::index_sequence<indexes...>, Children &&...children) {
	(prop::detail::add_child(container, std::forward<Children>(children)), ...);
}
