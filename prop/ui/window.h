#pragma once

#include "prop/ui/widget.h"
#include "prop/utility/polywrap.h"
#include "prop/utility/property.h"
#include "prop/utility/rect.h"

#include <memory>
#include <string>

namespace prop {
	namespace platform {
		class Window;
	}
	class Window {
		public:
		struct Parameters {
			Property<prop::Size> size = prop::Size{800, 600};
			Property<std::string> title = "";
			Property<prop::Polywrap<prop::Widget>> widget;
		};
		Window();
		Window(Parameters &&parameters);
		Window(Window &&other) noexcept;
		Window &operator=(Window &&other) noexcept;

		prop::platform::Window &platform_window() const;

		Property<prop::Size> size;
		Property<std::string> title;
		Property<prop::Polywrap<prop::Widget>> widget;

		static void pump();
		static void exec();

		private:
		std::unique_ptr<prop::platform::Window, void (*)(prop::platform::Window *)> platform_window_;
	};
} // namespace prop
