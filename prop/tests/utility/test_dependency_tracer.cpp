#include "proplib/ui/button.h"
#include "proplib/ui/vertical_layout.h"
#include "proplib/ui/widget.h"
#include "proplib/utility/dependency_tracer.h"
#include "proplib/utility/property.h"

#include <catch2/catch_all.hpp>

//TODO: Replace with reflection
template <class T>
constexpr std::size_t get_number_of_members() {
	if (std::is_same_v<T, prop::Widget::Parameters>) {
		return 5;
	}
}
template <class T>
constexpr std::size_t number_of_members = get_number_of_members<T>();

static std::size_t count_substrings(std::string_view string, std::string_view substring) {
	std::size_t count{};
	for (std::size_t pos = string.find(substring); pos != string.npos; pos = string.find(substring, pos + 1)) {
		count++;
	}
	return count;
}

TEST_CASE("Single property", "[Dependency_tracer]") {
	prop::Property p = 42;
	prop::Dependency_tracer tracer;
	PROP_TRACE(tracer, p);
	REQUIRE(tracer.object_data.contains(&p));
	REQUIRE(tracer.object_data[&p].name == "p");
	REQUIRE(p.type() == prop::type_name<decltype(p)>());
}

TEST_CASE("Multiple properties", "[Dependency_tracer]") {
	prop::Property p = 42;
	prop::Property pd = 3.14;
	prop::Dependency_tracer tracer;
	PROP_TRACE(tracer, p, pd);
	REQUIRE(tracer.object_data.contains(&p));
	REQUIRE(tracer.object_data[&p].name == "p");
	REQUIRE(p.type() == prop::type_name<decltype(p)>());
	REQUIRE(tracer.object_data.contains(&pd));
	REQUIRE(tracer.object_data[&pd].name == "pd");
	REQUIRE(pd.type() == prop::type_name<decltype(pd)>());
}

TEST_CASE("Property with dependents", "[Dependency_tracer]") {
	prop::Property p = 42;
	prop::Property pd = [&p] { return static_cast<double>(p); };
	REQUIRE(p.is_dependency_of(pd));
	REQUIRE(p.is_implicit_dependency_of(pd));
	REQUIRE(pd.is_dependent_on(p));
	WHEN("Adding only dependency") {
		prop::Dependency_tracer tracer;
		PROP_TRACE(tracer, p);
		REQUIRE(tracer.object_data.contains(&p));
		REQUIRE(tracer.object_data[&p].name == "p");
		REQUIRE(p.type() == prop::type_name<decltype(p)>());
		REQUIRE(tracer.object_data.contains(&pd));
		REQUIRE(tracer.object_data[&pd].name == "");
#ifdef PROPERTY_NAMES
		REQUIRE(pd.type() == prop::type_name<decltype(pd)>());
#else
		REQUIRE(tracer.object_data[&pd].type == "");
#endif
	}
	WHEN("Adding only dependent") {
		prop::Dependency_tracer tracer;
		PROP_TRACE(tracer, pd);
		REQUIRE(tracer.object_data.contains(&p));
		REQUIRE(tracer.object_data[&p].name == "");
#ifdef PROPERTY_NAMES
		REQUIRE(p.type() == prop::type_name<decltype(p)>());
#else
		REQUIRE(tracer.object_data[&p].type == "");
#endif
		REQUIRE(tracer.object_data.contains(&pd));
		REQUIRE(tracer.object_data[&pd].name == "pd");
		REQUIRE(pd.type() == prop::type_name<decltype(pd)>());
	}
}

TEST_CASE("Basic widget", "[Dependency_tracer]") {
	prop::Widget w;
	prop::Dependency_tracer tracer;
	PROP_TRACE(tracer, w);
	REQUIRE(tracer.object_data.contains(&w));
	REQUIRE(tracer.object_data[&w].name == "w");
	REQUIRE(tracer.object_data[&w].widget_data.size() == 1);
	REQUIRE(w.type() == prop::type_name<decltype(w)>());
	REQUIRE(tracer.object_data[&w].widget_data[w.type()].children.size() == 0);
	REQUIRE(tracer.object_data[&w].widget_data[w.type()].members.size() == number_of_members<prop::Widget::Parameters>);
	REQUIRE(tracer.object_data[&w.position].name == "position");
}

TEST_CASE("Derived widget", "[Dependency_tracer]") {
	prop::Button b;
	prop::Dependency_tracer tracer;
	PROP_TRACE(tracer, b);
	REQUIRE(tracer.object_data.contains(&b));
	auto &data = tracer.object_data[&b];
	REQUIRE(data.name == "b");
	REQUIRE(data.widget_data.size() == 2);
	REQUIRE(data.widget_data[0].type == prop::type_name<decltype(b)>());
	REQUIRE(data.widget_data[1].type == "prop::Widget");
}

TEST_CASE("Derived widget via base", "[Dependency_tracer]") {
	prop::Button b;
	prop::Widget &w = b;
	prop::Dependency_tracer tracer;
	PROP_TRACE(tracer, w);
	INFO(tracer.to_string());
	REQUIRE(tracer.object_data.contains(&b));
	auto &data = tracer.object_data[&b];
	REQUIRE(data.name == "w");
	REQUIRE(data.widget_data.size() == 2);
	const auto &button_data = data.widget_data.front();
	REQUIRE(button_data.type == prop::type_name<decltype(b)>());
	//REQUIRE(contains(button_data.members, {&b.font, "prop::Button"}));
	//REQUIRE(not contains(button_data.members, &w.position));
	const auto &widget_data = data.widget_data.back();
	REQUIRE(widget_data.type == "prop::Widget");
	//REQUIRE(contains(widget_data.members, &w.position));
	//REQUIRE(not contains(widget_data.members, &b.font));
}

TEST_CASE("Child widget", "[Dependency_tracer]") {
	prop::Widget w1, w2;
	prop::Vertical_layout vl{&w1, &w2};
	prop::Dependency_tracer tracer;
	PROP_TRACE(tracer, vl);
	INFO(tracer.to_string());
	REQUIRE(tracer.object_data.contains(&vl));
	REQUIRE(tracer.object_data.contains(&w1));
	REQUIRE(tracer.object_data.contains(&w2));
	auto &data = tracer.object_data[&vl];
	REQUIRE(data.name == "vl");
	REQUIRE(data.widget_data.size() == 2);
	const auto &widget_data = data.widget_data.back();
	REQUIRE(widget_data.type == prop::type_name<decltype(w1)>());
	const auto &vl_data = data.widget_data.front();
	const auto &vl_children = vl_data.children;
	REQUIRE(vl_children.size() == 2);
	REQUIRE(vl_children[0] == &w1);
	REQUIRE(vl_children[1] == &w2);
}

TEST_CASE("Trace printing properties", "[Dependency_tracer]") {
	prop::Property p = 42;
	WHEN("Single property") {
		auto trace = PROP_TRACER(p).to_string();
		INFO(trace);
		//TODO: Should probably be more specific about where and how those strings need to exist, maybe split on ANSI escape codes?
		REQUIRE(trace.contains("p"));
		REQUIRE(trace.contains("42"));
		REQUIRE(trace.contains(prop::to_string(std::addressof(p))));
	}
	WHEN("Linked property") {
		prop::Property p2 = [&p] { return p + 1; };
		auto trace = PROP_TRACER(p).to_string();
		INFO(trace);
		REQUIRE(trace.contains("p"));
		REQUIRE(trace.contains("42"));
		REQUIRE(trace.contains(prop::to_string(std::addressof(p))));
		REQUIRE(trace.contains("42"));
		REQUIRE(trace.contains("43"));
		REQUIRE(trace.contains(prop::to_string(std::addressof(p2))));
	}
}

TEST_CASE("Trace printing widgets", "[Dependency_tracer]") {
	WHEN("Widget") {
		prop::Widget w;
		auto trace = PROP_TRACER(w).to_string();
		INFO(trace);
		REQUIRE(trace.contains("w"));
		REQUIRE(count_substrings(trace, "position") == 1);
		REQUIRE(count_substrings(trace, "min_size") == 1);
		REQUIRE(count_substrings(trace, "max_size") == 1);
		REQUIRE(trace.contains(prop::to_string(std::addressof(w))));
	}
	WHEN("Derived widget") {
		prop::Button b;
		prop::Widget &w = b;
		auto trace = PROP_TRACER(w).to_string();
		INFO(trace);
		REQUIRE(trace.contains("w"));
		REQUIRE(trace.contains("prop::Widget"));
		REQUIRE(trace.contains("prop::Button"));
		REQUIRE(trace.contains("position"));
		REQUIRE(trace.contains("min_size"));
		REQUIRE(trace.contains("max_size"));
		REQUIRE(trace.contains(prop::to_string(std::addressof(w))));
		REQUIRE(trace.contains(prop::to_string(std::addressof(b))));
	}
}

TEST_CASE("Generating dot file", "[Dependency_tracer]") {
	//TODO: Find a way to test dot file generation
	WHEN("Displaying bound properties") {
		prop::Property p = 42;
		prop::Property p2 = [&p] { return p + 1; };
		REQUIRE(p2.is_dependent_on(p));
		REQUIRE(p.is_dependency_of(p2));
		PROP_TRACER(p).to_image();
	}
	WHEN("Displaying inherited widget") {
		prop::Button b;
		prop::Widget &button = b;
		PROP_TRACER(button).to_image();
	}
}
