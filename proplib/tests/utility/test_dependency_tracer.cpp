#include "proplib/ui/button.h"
#include "proplib/ui/widget.h"
#include "proplib/utility/dependency_tracer.h"
#include "proplib/utility/property.h"
#include "proplib/utility/utility.h"

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
	prop::Property pd = [&p] { return static_cast<double>(p); };
	REQUIRE(p.is_dependency_of(pd));
	REQUIRE(p.is_implicit_dependency_of(pd));
	REQUIRE(pd.is_dependent_on(p));
	WHEN("Adding only dependency") {
		prop::Dependency_tracer tracer;
		PROP_TRACE(tracer, p);
		REQUIRE(tracer.properties.contains(&p));
		REQUIRE(tracer.properties[&p].name == "p");
		REQUIRE(tracer.properties[&p].type == "int");
		REQUIRE(tracer.properties.contains(&pd));
		REQUIRE(tracer.properties[&pd].name == "");
#ifdef PROPERTY_NAMES
		REQUIRE(tracer.properties[&pd].type == "double");
#else
		REQUIRE(tracer.properties[&pd].type == "");
#endif
	}
	WHEN("Adding only dependent") {
		prop::Dependency_tracer tracer;
		PROP_TRACE(tracer, pd);
		REQUIRE(tracer.properties.contains(&p));
		REQUIRE(tracer.properties[&p].name == "");
#ifdef PROPERTY_NAMES
		REQUIRE(tracer.properties[&p].type == "int");
#else
		REQUIRE(tracer.properties[&p].type == "");
#endif
		REQUIRE(tracer.properties.contains(&pd));
		REQUIRE(tracer.properties[&pd].name == "pd");
		REQUIRE(tracer.properties[&pd].type == "double");
	}
}

TEST_CASE("Basic widget") {
	prop::Widget w;
	prop::Dependency_tracer tracer;
	PROP_TRACE(tracer, w);
	REQUIRE(tracer.widgets.contains(&w));
	REQUIRE(tracer.widgets[&w].name == "w");
	REQUIRE(tracer.widgets[&w].data.size() == 1);
	REQUIRE(tracer.widgets[&w].data.front().type == "prop::Widget");
	REQUIRE(tracer.properties.contains(&w.rect));
	REQUIRE(tracer.properties[&w.rect].type == "prop::Rect");
	REQUIRE(tracer.properties[&w.rect].name == "rect");
	REQUIRE(tracer.properties[&w.rect].widget == &w);
}

TEST_CASE("Derived widget") {
	prop::Button b;
	prop::Dependency_tracer tracer;
	PROP_TRACE(tracer, b);
	REQUIRE(tracer.widgets.contains(&b));
	auto &data = tracer.widgets[&b];
	REQUIRE(data.name == "b");
	REQUIRE(data.data.size() == 2);
	REQUIRE(data.data.front().type == "prop::Button");
	REQUIRE(data.data.back().type == "prop::Widget");
}

TEST_CASE("Derived widget via base") {
	prop::Button b;
	prop::Widget &w = b;
	prop::Dependency_tracer tracer;
	PROP_TRACE(tracer, w);
	tracer.print_widget_trace(std::cout);
	REQUIRE(tracer.widgets.contains(&b));
	auto &data = tracer.widgets[&b];
	REQUIRE(data.name == "w");
	REQUIRE(data.data.size() == 2);
	const auto &button_data = data.data.front();
	REQUIRE(button_data.type == "prop::Button");
	REQUIRE(contains(button_data.properties, &b.font));
	REQUIRE(not contains(button_data.properties, &w.rect));
	const auto &widget_data = data.data.back();
	REQUIRE(widget_data.type == "prop::Widget");
	REQUIRE(contains(widget_data.properties, &w.rect));
	REQUIRE(not contains(widget_data.properties, &b.font));
}
