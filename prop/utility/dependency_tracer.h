#pragma once

#include "alignment.h"
#include "prop/ui/widget.h"
#include "property_link.h"
#include "type_name.h"

#include <filesystem>
#include <map>
#include <ranges>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

#define PROP_TRACE(PROP_TRACER, ...) PROP_TRACER.trace(#__VA_ARGS__ __VA_OPT__(, ) __VA_ARGS__)
#define PROP_TRACER(...)                                                                                               \
	[&] {                                                                                                              \
		prop::Dependency_tracer PROP_tracer;                                                                           \
		PROP_tracer.trace(#__VA_ARGS__ __VA_OPT__(, ) __VA_ARGS__);                                                    \
		return PROP_tracer;                                                                                            \
	}()

namespace prop {
	class Widget;
	class Dependency_tracer {
		public:
		using Link_id = prop::Property_link::Property_pointer;

		struct Non_link_member {
			std::string_view type;
			std::string value;
			const void *address{};
		};
		struct Member_data {
			std::string_view name;
			std::variant<Non_link_member, const prop::Property_link *> data;
			std::string_view get_type() const {
				return std::visit(
					[](auto &&m) {
						if constexpr (std::is_same_v<std::remove_cvref_t<decltype(m)>,
													 prop::Dependency_tracer::Non_link_member>) {
							return m.type;
						} else {
							return m->type();
						}
					},
					data);
			}
			std::string get_value_string() const {
				return std::visit(
					[](auto &&m) {
						if constexpr (std::is_same_v<std::remove_cvref_t<decltype(m)>,
													 prop::Dependency_tracer::Non_link_member>) {
							return m.value;
						} else {
							return m->value_string();
						}
					},
					data);
			}
			const void *get_address() const {
				return std::visit(
					[](auto &&m) -> const void * {
						if constexpr (std::is_same_v<std::remove_cvref_t<decltype(m)>,
													 prop::Dependency_tracer::Non_link_member>) {
							return m.address;
						} else {
							return m;
						}
					},
					data);
			}
		};
		struct Widget_data {
			std::string_view type;
			std::vector<Member_data> members;
			std::vector<const prop::Widget *> children;
			const Property_link *link{};
		};
		struct Widget_data_container {
			Widget_data &operator[](std::string_view type) {
				for (auto &w : data) {
					if (w->type == type) {
						return *w;
					}
				}
				data.push_back(std::make_unique<Widget_data>(type));
				return *data.back();
			}
			Widget_data &operator[](std::size_t index) {
				assert(data.size() > index);
				return *data[index];
			}
			std::size_t size() const {
				return data.size();
			}
			bool is_empty() const {
				return !data.size();
			}
			Widget_data &front() {
				assert(not data.empty());
				return *data.front();
			}
			Widget_data &back() {
				assert(not data.empty());
				return *data.back();
			}
			auto begin() const {
				return std::begin(data);
			}
			auto end() const {
				return std::end(data);
			}

			private:
			std::vector<std::unique_ptr<Widget_data>> data;
		};

		struct Object_data {
			std::string_view name;
			Widget_data *parent{};
			Widget_data_container widget_data;
		};

		std::map<const prop::Property_link *, Object_data> object_data;

		struct Make_current {
			template <class Widget>
				requires(std::is_convertible_v<Widget *, prop::Widget *>)
			Make_current(const Widget &widget, Dependency_tracer &tracer)
				: previous_object{tracer.current_widget}
				, dependency_tracer{&tracer} {
				assert(tracer.object_data.contains(&widget));
				tracer.current_widget =
					&tracer.object_data[&widget].widget_data[prop::type_name<std::remove_cvref_t<Widget>>()];
				tracer.current_widget->link = &widget;
			}
			Make_current(const Make_current &) = delete;
			~Make_current() {
				dependency_tracer->current_widget = previous_object;
			}

			private:
			Widget_data *previous_object;
			Dependency_tracer *dependency_tracer;
		};

		template <class... Args>
		void trace(std::string_view names, const Args &...args) {
			std::ranges::split_view split_names(names, std::string_view{", "});
			auto current_name = std::begin(split_names);
			(..., add(std::string_view{*current_name++}, args));
		}
		template <class Widget>
			requires(std::is_convertible_v<Widget &, prop::Widget &>)
		void trace(const Widget &widget) {
			add("", widget);
		}
		std::string to_string() const;
		void to_image(std::filesystem::path output_path = std::filesystem::temp_directory_path()) const;

		static std::intptr_t heap_base_address;
		static std::intptr_t stack_base_address;
		static std::intptr_t global_base_address;

		private:
		template <class T>
		void add(std::string_view name, const T &p) {
			if constexpr (std::is_convertible_v<T &, const prop::Property_link *>) {
				auto property_link_pointer = static_cast<const prop::Property_link *>(p);
				if (property_link_pointer) {
					add(name, *property_link_pointer);
				}
				return;
			}
			if constexpr (std::is_convertible_v<T &, const prop::Property_link &>) {
				if (auto widget = dynamic_cast<const prop::Widget *>(&static_cast<const prop::Property_link &>(p))) {
					if (current_widget) {
						if (std::find(std::begin(current_widget->children), std::end(current_widget->children),
									  widget) == std::end(current_widget->children)) {
							current_widget->children.push_back(widget);
						}
					}
					auto [it, inserted] = object_data.insert({
						widget,
						{
							.name = name,
							.parent = current_widget,
							.widget_data = {},
						},
					});
					if (inserted) {
						widget->trace(*this);
					}
					if (name != "") {
						auto &object_name = it->second.name;
						if (object_name == "") {
							object_name = name;
						}
					}
				} else { //not a widget
					auto [it, inserted] = object_data.insert({
						&p,
						{
							.name = name.empty() ? static_cast<const prop::Property_link &>(p).custom_name : name,
							.parent = current_widget,
							.widget_data = {},
						},
					});
					if (inserted) {
						if (current_widget) {
							auto previous_widget = current_widget;
							current_widget = nullptr;
							for (auto &dep : it->first->dependencies) {
								add("", dep);
							}
							current_widget = previous_widget;
							current_widget->members.push_back({.name = name, .data = it->first});
						} else {
							for (auto &dep : it->first->dependencies) {
								add("", dep);
							}
						}
					} else if (current_widget and it->second.parent == nullptr) {
						it->second.parent = current_widget;
						current_widget->members.push_back({
							.name = name,
							.data = it->first,
						});
						it->second.name = name;
					}
				}
			} else {
				assert(current_widget);
				current_widget->members.push_back({
					.name = name,
					.data =
						Non_link_member{
							.type = prop::type_name<T>(),
							.value = prop::to_string(p),
							.address = std::addressof(p),
						},
				});
			}
		}

		std::string dot_name(const Property_link *link, prop::Alignment alignment = none) const;

		Widget_data *current_widget = nullptr;
	};
}; // namespace prop
