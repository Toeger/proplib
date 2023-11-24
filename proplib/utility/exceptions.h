#pragma once

#include <stdexcept>

namespace prop {
	struct Copy_error : std::runtime_error {
		using std::runtime_error::runtime_error;
	};

	struct Logic_error : std::logic_error {
		using std::logic_error::logic_error;
	};
} // namespace prop
