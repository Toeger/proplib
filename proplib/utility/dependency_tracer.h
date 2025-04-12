#pragma once

#include "alignment.h"
#include "property.h"
#include "proplib/ui/widget.h"
#include "type_name.h"
#include "type_traits.h"
#include "utility.h"
#include <filesystem>
#include <map>
#include <ostream>
#include <ranges>
#include <string_view>
#include <type_traits>

#define PROP_TRACE(PROP_TRACER, ...) PROP_TRACER.trace(#__VA_ARGS__ __VA_OPT__(, ) __VA_ARGS__)
#define PROP_TRACER(...)                                                                                               \
	[&] {                                                                                                              \
		prop::Dependency_tracer tracer;                                                                                \
		tracer.trace(#__VA_ARGS__ __VA_OPT__(, ) __VA_ARGS__);                                                         \
		return tracer;                                                                                                 \
	}()

namespace prop {
	class Dependency_tracer {
		struct Widget_id {
			Widget_id(const prop::Widget *w)
				: widget_id{dynamic_cast<const void *>(w)} {}
			operator const void *() const {
				return widget_id;
			}
			auto operator<=>(const Widget_id &) const = default;
			const void *widget_id;
		};

		public:
		struct Make_current {
			Make_current(std::nullptr_t, Dependency_tracer &tracer)
				: previous_widget{tracer.current_widget}
				, previous_sub_widget_type{previous_widget ? tracer.current_sub_widget->type : ""}
				, dependency_tracer{&tracer} {
				tracer.current_widget = nullptr;
				tracer.current_sub_widget = nullptr;
			}
			template <class Widget>
				requires(std::is_base_of_v<prop::Widget, Widget>)
			Make_current(const Widget *widget, Dependency_tracer &tracer)
				: previous_widget{tracer.current_widget}
				, previous_sub_widget_type{previous_widget ? tracer.current_sub_widget->type : ""}
				, dependency_tracer{&tracer} {
				tracer.current_widget = widget;
				auto sub_widget_type = prop::type_name<Widget>();
				auto &widget_object_data = tracer.widgets[(const prop::Widget *)widget];
				tracer.current_sub_widget = nullptr;
				for (auto &base : widget_object_data.bases) {
					if (base.type == sub_widget_type) {
						tracer.current_sub_widget = &base;
						break;
					}
				}
				if (not tracer.current_sub_widget) {
					widget_object_data.bases.push_back({
						.type = sub_widget_type,
					});
					tracer.current_sub_widget = &widget_object_data.bases.back();
				}
			}
			Make_current(const Make_current &) = delete;
			~Make_current() {
				dependency_tracer->current_widget = previous_widget;
				if (not previous_widget) {
					dependency_tracer->current_sub_widget = nullptr;
					return;
				}
				auto &widget_object_data = dependency_tracer->widgets[previous_widget];
				for (auto &base : widget_object_data.bases) {
					if (base.type == previous_sub_widget_type) {
						dependency_tracer->current_sub_widget = &base;
					}
				}
			}

			private:
			const prop::Widget *previous_widget;
			std::string_view previous_sub_widget_type;
			Dependency_tracer *dependency_tracer;
		};

		struct Widget_base_data {
			std::string_view type;
			std::vector<const void *> properties;
			std::vector<const prop::Widget *> children;
		};
		struct Widget_object_data {
			std::string_view name;
			std::vector<Widget_base_data> bases;
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

		std::map<Widget_id, Widget_object_data> widgets;
		std::map<const void *, Property_data> properties;
		void print_widget_trace(std::ostream &os) const;
		void to_image(std::filesystem::path output_path) const;

		template <class Widget>
			requires std::is_base_of_v<prop::Widget, Widget>
		void trace_base(const Widget *widget) {
			widget->Widget::trace(*this);
		}
		void trace_child(const prop::Widget &widget) {
			if (not widgets.contains(&widget)) {
				widgets[&widget] = {
					.name = "<unnamed>",
				};
				widget.trace(*this);
			}
			if (current_sub_widget) {
				current_sub_widget->children.push_back(&widget);
			}
		}

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
			requires std::is_base_of_v<prop::Widget, Widget>
		void add(std::string_view name, const Widget &widget) {
			trace_child(widget);
			widgets[&widget].name = name;
		}

		void add(const prop::detail::Property_base *pb);
		std::string dot_name(const void *p, prop::Alignment alignment = none) const;

		const prop::Widget *current_widget = nullptr;
		Widget_base_data *current_sub_widget = nullptr;
	};
}; // namespace prop
