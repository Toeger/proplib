#pragma once

#include "../utility/polywrap.h"
#include "../utility/property.h"
#include "widget.h"

#include <memory>
#include <string>

namespace prop {
	class Window {
		public:
		Window(std::string title = {}, int width = 800, int height = 600);
		~Window();
		Property<int> width;
		Property<int> height;
		Property<std::string> title;
		Property<prop::Polywrap<prop::Widget>> widget;

		static void pump();
		static void exec();

		std::unique_ptr<struct Window_privates> privates;
	};
} // namespace prop
