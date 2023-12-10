#include "../utility/signal.h"

#include <catch2/catch.hpp>

TEST_CASE("Default constructed") {
	int callcount = 0;
	prop::Signal s;
	s.connect([&callcount] { callcount++; });
	REQUIRE(callcount == 0);
	s.emit();
	REQUIRE(callcount == 1);
	s.emit();
	REQUIRE(callcount == 2);
}

TEST_CASE("Single argument") {
	int value = 0;
	prop::Signal<int> s;
	s.connect([&value](int i) { value = i; });
	REQUIRE(value == 0);
	s.emit(42);
	REQUIRE(value == 42);
	s.emit(38);
	REQUIRE(value == 38);
}

TEST_CASE("Convertible argument type") {
	int value = 0;
	prop::Signal<int> s;
	s.connect([&value](double d) { value = d; });
	REQUIRE(value == 0);
	s.emit(42);
	REQUIRE(value == 42);
	s.emit(38);
	REQUIRE(value == 38);
}

TEST_CASE("Multiple connections") {
	int callcount = 0;
	prop::Signal s;
	s.connect([&callcount] { callcount++; });
	REQUIRE(callcount == 0);
	s.emit();
	REQUIRE(callcount == 1);
	s.connect([&callcount] { callcount++; });
	s.emit();
	REQUIRE(callcount == 3);
	s.connect([&callcount] { callcount++; });
	s.emit();
	REQUIRE(callcount == 6);
}

TEST_CASE("Connecting signal to signal") {}

TEST_CASE("Connection with extra properties") {}

TEST_CASE("Disconnecting all connections") {
	int callcount = 0;
	prop::Signal s;
	s.connect([&callcount] { callcount++; });
	s.disconnect_all();
	s.emit();
	REQUIRE(callcount == 0);
}

TEST_CASE("Disconnecting specific connections") {}

TEST_CASE("Move signal") {}
