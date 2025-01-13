#pragma once

#include "property.h"
#include "proplib/ui/widget.h"
#include "type_name.h"
#include "type_traits.h"
#include <concepts>
#include <map>
#include <ranges>

#define PROP_TRACE(PROP_TRACER, ...) PROP_TRACER.trace(#__VA_ARGS__ __VA_OPT__(, ) __VA_ARGS__)

namespace prop {
	class Dependency_tracer {
		public:
		struct Widget_data {
			std::string_view type;
			std::string_view name;
			std::vector<const void *> properties;
			std::vector<const prop::Widget *> children;
		};
		struct Property_data {
			std::string_view type;
			std::string_view name;
			std::vector<const void *> dependents;
			std::vector<const void *> dependencies;
		};

		template <class... Args>
		void trace(std::string_view names, const Args &...args);

		std::multimap<const prop::Widget *, Widget_data> widgets;
		std::map<const void *, Property_data> properties;

		private:
		template <class T>
		void add(std::string_view name, const prop::Property<T> &property);
		template <class Widget>
			requires(std::is_base_of_v<prop::Widget, Widget>)
		void add(std::string_view name, const Widget &property);
		void add(const prop::detail::Property_base *pb);
	};
}; // namespace prop

template <class... Args>
void prop::Dependency_tracer::trace(std::string_view names, const Args &...args) {
	std::ranges::split_view split_names(names, std::string_view{", "});
	auto current_name = std::begin(split_names);
	(add(std::string_view{*current_name++}, args), ...);
}

template <class T>
void prop::Dependency_tracer::add(std::string_view name, const prop::Property<T> &property) {
	add(prop::detail::get_property_base_pointer(property));
	auto &prop = properties[&property];
#ifndef PROPERTY_NAMES
	prop.type = prop::type_name<T>();
#endif
	if (not name.empty()) {
		properties[&property].name = name;
	}
}

inline void prop::Dependency_tracer::add(const prop::detail::Property_base *pb) {
	if (properties.contains(pb)) {
		return;
	}
	properties[pb] = {
#ifdef PROPERTY_NAMES
		.type = pb->type,
#else
		.type = "",
#endif
		.name = "",
		.dependents = {std::begin(pb->dependents), std::end(pb->dependents)},
		.dependencies = {std::begin(pb->implicit_dependencies), std::end(pb->implicit_dependencies)},
	};
	for (auto deps : pb->dependents) {
		add(deps);
	}
	for (auto deps : pb->implicit_dependencies) {
		add(deps);
	}
}
