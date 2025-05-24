#include "utility.h"

std::ostream &prop::operator<<(std::ostream &os, const Self &self) {
	return os << self.self;
}
