#include "platform_xrandr_screen.h"

#include <cstdio>
#include <memory>
#include <mutex>
#include <regex>

//TODO: Extract to utility file
static std::string get_output_from_command(std::string_view command) {
	std::unique_ptr<FILE, decltype([](FILE *f) { pclose(f); })> fp;
	{
		static std::mutex popen_mutex;
		std::lock_guard<std::mutex> _(popen_mutex); //popen doesn't seem to be thread-safe
		fp.reset(popen(command.data(), "r"));
	}
	if (!fp) {
		return {};
	}
	std::string buffer;
	const int buffersize = 1024;
	for (;;) {
		buffer.resize(buffer.size() + buffersize);
		const std::size_t read =
			fread(&buffer[buffer.size() - buffersize], sizeof *buffer.data(), buffersize, fp.get());
		if (read < buffersize) {
			buffer.resize(buffer.size() - buffersize + read);
			break;
		}
	}
	return buffer;
}

std::vector<prop::platform::Screen> get_xandr_screens() {
	std::vector<prop::platform::Screen> screens;
	auto output = get_output_from_command("xrandr --listactivemonitors");
	const std::regex r(R"(\s(\d+)\/(\d+)x(\d+)\/(\d+)\+(\d+)\+(\d+)\s)");
	for (std::smatch sm; regex_search(output, sm, r); output = sm.suffix()) {
		prop::platform::Screen screen;
		screen.width_pixels = std::atoi(sm[1].str().c_str());
		screen.height_pixels = std::atoi(sm[3].str().c_str());
		screen.x_origin_pixels = std::atoi(sm[5].str().c_str());
		screen.y_origin_pixels = std::atoi(sm[6].str().c_str());
		screen.x_dpi = screen.width_pixels / std::atoi(sm[2].str().c_str()) * 25.4l;
		screen.y_dpi = screen.height_pixels / std::atoi(sm[4].str().c_str()) * 25.4l;
		screens.push_back(std::move(screen));
	}
	return screens;
}
