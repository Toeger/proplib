#pragma once

#include "property.h"

#include <functional>

template <>
class prop::Property<void> : prop::detail::Property_base {
	public:
	Property(std::function<void()> source);
	Property &operator=(std::function<void()> source);
	void unbind();
	void update() override final;

	private:
	void update_source(std::function<void()> f);
	std::function<void()> source;
};

namespace prop {
	using Binding = prop::Property<void>;
} // namespace prop
