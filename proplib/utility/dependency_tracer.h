#pragma once

#include "property.h"
#include "proplib/ui/widget.h"
#include "type_name.h"
#include "utility.h"
#include <map>
#include <ostream>
#include <ranges>
#include <string_view>
#include <type_traits>

#define PROP_TRACE(PROP_TRACER, ...) PROP_TRACER.trace(#__VA_ARGS__ __VA_OPT__(, ) __VA_ARGS__)

namespace prop {
	class Dependency_tracer {
		public:
		struct Widget_sub_data {
			std::string_view type;
			std::vector<const void *> properties;
			std::vector<const prop::Widget *> children;
		};
		struct Widget_common_data {
			std::string_view name;
			std::vector<Widget_sub_data> data;
		};

		struct Property_data {
			std::string_view type;
			std::string_view name;
			std::vector<const void *> dependents;
			std::vector<const void *> dependencies;
			const prop::Widget *widget;
		};

		template <class... Args>
		void trace(std::string_view names, const Args &...args) {
			std::ranges::split_view split_names(names, std::string_view{", "});
			auto current_name = std::begin(split_names);
			(add(std::string_view{*current_name++}, args), ...);
		}

		std::map<const prop::Widget *, Widget_common_data> widgets;
		std::map<const void *, Property_data> properties;
		void print_widget_trace(std::ostream &os) const;

		private:
		template <class T>
		void add(std::string_view name, const prop::Property<T> &property) {
			add(prop::detail::get_property_base_pointer(property));
			auto &prop = properties[&property];
#ifndef PROPERTY_NAMES
			prop.type = prop::type_name<T>();
#endif
			properties[&property].name = name;
		}

		template <class Widget>
			requires std::is_base_of_v<::prop::Widget, Widget>
		void add(std::string_view name, const Widget &widget) {
			if (not widgets.contains(&widget)) {
				auto &data = widgets[&widget];
				widget.trace(*this);
				data.name = name;
				return;
			}
			auto &common_data = widgets[&widget];
			if (common_data.name.empty() or common_data.name == "*this") {
				common_data.name = name;
			}
			if (std::find_if(std::begin(common_data.data), std::end(common_data.data),
							 [](prop::Dependency_tracer::Widget_sub_data &wd) {
								 return wd.type == prop::type_name<Widget>();
							 }) != std::end(common_data.data)) {
				return;
			}
			common_data.data.push_back({
				.type = prop::type_name<Widget>(),
			});
			current_widget = {&widget, common_data.data.size() - 1};
		}

		void add(const prop::detail::Property_base *pb);
		std::pair<const prop::Widget *, std::size_t> current_widget = {nullptr, -1};
	};
}; // namespace prop
