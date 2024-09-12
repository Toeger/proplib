#pragma once

#include "utility/polywrap.h"
#include "utility/property.h"
#include "widget.h"

#include <memory>
#include <string>

namespace prop {
	class Window {
		public:
		struct Parameters {
			Property<int> width = 800;
			Property<int> height = 600;
			Property<std::string> title = "";
			Property<prop::Polywrap<prop::Widget>> widget;
		};
		Window();
		Window(Parameters &&parameters);
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
