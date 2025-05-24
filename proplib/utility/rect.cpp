#include "rect.h"

std::ostream &prop::operator<<(std::ostream &os, const Size &size) {
	return os << "[width=" << size.width << ", height=" << size.width << ']';
}

std::ostream &prop::operator<<(std::ostream &os, const prop::Rect &rect) {
	return os << "[top=" << rect.top << ", left=" << rect.left << ", bottom=" << rect.bottom << ", right=" << rect.right
			  << ']';
}
