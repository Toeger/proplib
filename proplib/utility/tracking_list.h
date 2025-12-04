#pragma once

#include "property_link.h"

namespace prop {
	class Tracking_list final : private prop::Property_link {
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
			Property_link::Property_pointer &operator*() const {
				return list->dependencies[index];
			}
			Property_link::Property_pointer *operator->() const {
				return &list->dependencies[index];
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
		template <class T>
			requires(std::is_base_of_v<T, prop::Property_link>)
		static Tracking_list for_explicit_dependencies(const T &linked) {
			return {std::begin(linked.dependencies) + linked.explicit_dependencies,
					std::begin(linked.dependencies) + linked.explicit_dependencies};
		}

		template <class T>
			requires(std::is_base_of_v<T, prop::Property_link>)
		static Tracking_list for_implicit_dependencies(const T &linked) {
			return {std::begin(linked.dependencies) + linked.explicit_dependencies,
					std::begin(linked.dependencies) + linked.explicit_dependencies};
		}

		template <class T>
			requires(std::is_base_of_v<T, prop::Property_link>)
		static Tracking_list for_all_dependencies(const T &linked) {
			return {std::begin(linked.dependencies) + linked.explicit_dependencies,
					std::begin(linked.dependencies) + linked.explicit_dependencies};
		}

		template <class T>
			requires(std::is_base_of_v<T, prop::Property_link>)
		static Tracking_list for_dependents(const T &linked) {
			return std::span<const prop::Property_link::Property_pointer>{
				std::begin(linked.dependencies) + linked.explicit_dependencies,
				std::begin(linked.dependencies) + linked.explicit_dependencies};
		}

		template <class T>
			requires(std::is_base_of_v<T, prop::Property_link>)
		static Tracking_list for_all_links(const T &linked) {
			return {std::begin(linked.dependencies) + linked.explicit_dependencies,
					std::begin(linked.dependencies) + linked.explicit_dependencies};
		}

		auto begin() const {
			return Tracking_list_iterator{this};
		}

		auto end() const {
			return nullptr;
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
