#include "proplib/utility/dependency_tracer.h"
#include "proplib/utility/property.h"

#include <catch2/catch_all.hpp>

TEST_CASE("Single property") {
	prop::Property p = 42;
	prop::Dependency_tracer tracer;
	PROP_TRACE(tracer, p);
	REQUIRE(tracer.properties.contains(&p));
	REQUIRE(tracer.properties[&p].name == "p");
	REQUIRE(tracer.properties[&p].type == "int");
}

TEST_CASE("Multiple properties") {
	prop::Property p = 42;
	prop::Property pd = 3.14;
	prop::Dependency_tracer tracer;
	PROP_TRACE(tracer, p, pd);
	REQUIRE(tracer.properties.contains(&p));
	REQUIRE(tracer.properties[&p].name == "p");
	REQUIRE(tracer.properties[&p].type == "int");
	REQUIRE(tracer.properties.contains(&pd));
	REQUIRE(tracer.properties[&pd].name == "pd");
	REQUIRE(tracer.properties[&pd].type == "double");
}

TEST_CASE("Property with dependents") {
	prop::Property p = 42;
	prop::Property<double> pd = [&p] { return static_cast<double>(p); };
	prop::Dependency_tracer tracer;
	PROP_TRACE(tracer, p);
	REQUIRE(tracer.properties.contains(&p));
	REQUIRE(tracer.properties[&p].name == "p");
	REQUIRE(tracer.properties[&p].type == "int");
	REQUIRE(tracer.properties.contains(&pd));
	REQUIRE(tracer.properties[&pd].name == "");
	REQUIRE(tracer.properties[&pd].type == "double");
}
