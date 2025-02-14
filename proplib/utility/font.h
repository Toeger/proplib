#pragma once

#include "proplib/utility/alignment.h"
#include "proplib/utility/color.h"

#include <optional>
#include <string>

namespace prop {
	class Font {
		struct Attributes {
			std::optional<prop::Color> color;
			std::optional<std::string> name;
			std::optional<prop::Alignment> alignment;
			std::optional<float> pixel_size; //point size?
			//std::optional<> clipping_mode;
			std::optional<bool> bold;
			std::optional<bool> italic;
			std::optional<bool> strikeout;
			std::optional<bool> subscript;
			std::optional<bool> superscript;
			std::optional<bool> underline;
		};

		public:
		Font();
		Font(const Font &) = default;
		Font(Font &&) = default;
		Font &operator=(const Font &) = default;
		Font &operator=(Font &&) = default;
		Font(Attributes attributes_);
		void set(Attributes attributes_);
		Font with(Attributes attributes_);

		prop::Color color;
		std::string name;
		prop::Alignment alignment;
		float pixel_size; //point size?
		//clipping_mode;
		bool bold = false;
		bool italic = false;
		bool strikeout = false;
		bool underline = false;
	};
} // namespace prop
