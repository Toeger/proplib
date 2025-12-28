#pragma once

#include <stdexcept>

namespace prop {
	//thrown when a Polywrap<CopyableBase> is being assigned a NonCopyableDerived and then the Polywrap<CopyableBase> is being copied
	struct Copy_error : std::runtime_error {
		using std::runtime_error::runtime_error;
	};

	//thrown on internal logic errors that should never happen
	struct Logic_error : std::logic_error {
		using std::logic_error::logic_error;
	};

	//thrown when a required explicit dependency is unavailable, caught internally
	struct Property_expired : std::runtime_error {
		using std::runtime_error::runtime_error;
	};

	//thrown when a given font could not be loaded
	struct Io_error : std::runtime_error {
		using std::runtime_error::runtime_error;
	};
} // namespace prop
