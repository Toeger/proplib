#pragma once

#include "property_link.h"

namespace prop {
	template <class T = prop::Property_link>
		requires(std::is_convertible_v<T *, prop::Property_link *>)
	class Tracking_list final : public prop::Property_link {
		private:
		struct Tracking_list_iterator {
			Tracking_list_iterator(const Tracking_list *Tracking_list, std::size_t pos = 0)
				: list{Tracking_list}
				, index{pos} {}
			Tracking_list_iterator(const Tracking_list_iterator &) = delete;
			operator bool() const {
				return index < list->explicit_dependencies + list->implicit_dependencies;
			}
			Tracking_list_iterator &operator++() {
				index++;
				return *this;
			}
			Tracking_list_iterator operator++(int) {
				auto pos = index;
				++*this;
				return Tracking_list_iterator{list, pos};
			}
			Property_link::Property_pointer &operator*() const
				requires(std::is_same_v<T, prop::Property_link>)
			{
				return list->dependencies[index];
			}
			Property_link::Property_pointer *operator->() const
				requires(std::is_same_v<T, prop::Property_link>)
			{
				return &list->dependencies[index];
			}
			T &operator*() const
				requires(not std::is_same_v<T, prop::Property_link>)
			{
				return dynamic_cast<T &>(list->dependencies[index]);
			}
			Property_link::Property_pointer *operator->() const
				requires(not std::is_same_v<T, prop::Property_link>)
			{
				return dynamic_cast<T *>(&list->dependencies[index]);
			}
			bool operator==(std::nullptr_t) const {
				return not *this;
			}

			private:
			const Tracking_list *list;
			std::size_t index = 0;
		};

		using prop::Property_link::dependencies;
		using prop::Property_link::explicit_dependencies;
		using prop::Property_link::implicit_dependencies;

		public:
		Tracking_list(std::span<const Property_link::Property_pointer> list)
			: prop::Property_link{{std::begin(list), std::end(list)}} {}

		static Tracking_list of_explicit_dependencies(const prop::Property_link &linked) {
			return {std::begin(linked.dependencies), std::begin(linked.dependencies) + linked.explicit_dependencies};
		}

		static Tracking_list of_implicit_dependencies(const prop::Property_link &linked) {
			return {std::begin(linked.dependencies) + linked.explicit_dependencies,
					std::begin(linked.dependencies) + linked.explicit_dependencies + linked.implicit_dependencies};
		}

		static Tracking_list of_all_dependencies(const prop::Property_link &linked) {
			return {std::begin(linked.dependencies),
					std::begin(linked.dependencies) + linked.explicit_dependencies + linked.implicit_dependencies};
		}

		static Tracking_list of_dependents(const prop::Property_link &linked) {
			return std::span<const prop::Property_link::Property_pointer>{
				std::begin(linked.dependencies) + linked.explicit_dependencies + linked.implicit_dependencies,
				std::end(linked.dependencies)};
		}

		static Tracking_list of_all_links(const prop::Property_link &linked) {
			return {std::begin(linked.dependencies), std::end(linked.dependencies)};
		}

		template <class... Args>
			requires(std::is_convertible_v<std::remove_reference_t<Args> *, prop::Property_link *> and ...)
		static Tracking_list of(Args &&...args) {
			return std::span<const prop::Property_link::Property_pointer>{{std::addressof(args), false}...};
		}

		template <class Container>
			requires(std::is_convertible_v<typename Container::value_type *, prop::Property_link *>)
		static Tracking_list<typename Container::value_type> from_container(Container &c) {
			return {std::begin(c), std::end(c)};
		}

		auto begin() const {
			return Tracking_list_iterator{this};
		}

		auto end() const {
			return nullptr;
		}

		const prop::Property_link *operator[](std::size_t index) const {
			return dependencies[index];
		}

		std::string_view type() const override {
			return "Tracking_list";
		}

		std::string value_string() const override {
			return "Tracking_list";
		}

		bool has_source() const override {
			return false;
		}

		void update() override {}
	};
} // namespace prop
