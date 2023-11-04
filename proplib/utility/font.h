#pragma once

#include <filesystem>
#include <memory>

namespace prop {
	class Font {
		public:
		Font();
		Font(const Font &other);
		Font(Font &&other);
		Font &operator=(const Font &other);
		Font &operator=(Font &&other);
		~Font();
		bool load(std::filesystem::path path);
		struct Font_privates &privates();
		const struct Font_privates &privates() const;

		std::unique_ptr<struct Font_privates> font_privates;
	};
} // namespace prop
