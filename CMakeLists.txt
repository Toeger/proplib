cmake_minimum_required(VERSION 3.12)

project(PropDemo VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS} -ggdb -fno-omit-frame-pointer -fsanitize=undefined,address")
set(CMAKE_LINKER_FLAGS_DEBUG  "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined,address")

#TODO: Make a prop library
set(PROP_LIBRARY_UI_SOURCES
	button
	label
	widget
	window
	vertical_layout
)
list(TRANSFORM PROP_LIBRARY_UI_SOURCES PREPEND "proplib/ui/")

set(PROP_LIBRARY_UTILITY_SOURCES
	binding
	font
	polywrap
	property
	raii
	style
	type_name
	type_traits
)
list(TRANSFORM PROP_LIBRARY_UTILITY_SOURCES PREPEND "proplib/utility/")

set(PROP_LIBRARY_SOURCES
	${PROP_LIBRARY_UI_SOURCES}
	${PROP_LIBRARY_UTILITY_SOURCES}
)
list(TRANSFORM PROP_LIBRARY_SOURCES APPEND ".cpp")

list(TRANSFORM PROP_LIBRARY_SOURCES REPLACE "\.cpp$" ".h" OUTPUT_VARIABLE PROP_LIBRARY_HEADERS)

list(TRANSFORM PROP_LIBRARY_SOURCES REPLACE "\.cpp$" ".privates.h" OUTPUT_VARIABLE PROP_LIBRARY_PRIVATE_CANDIDATES)
foreach(PROP_LIBRARY_PRIVATES_FILE IN ITEMS ${PROP_LIBRARY_PRIVATE_CANDIDATES})
	if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${PROP_LIBRARY_PRIVATES_FILE}")
		set(PROP_LIBRARY_PRIVATES ${PROP_LIBRARY_PRIVATES} ${PROP_LIBRARY_PRIVATES_FILE})
	endif()
endforeach()

#demo
find_package(SFML COMPONENTS window graphics system)
add_executable(PropDemo
	demo/main.cpp
	${PROP_LIBRARY_SOURCES}
	${PROP_LIBRARY_HEADERS}
	${PROP_LIBRARY_PRIVATES} #this doesn't belong to the demo, but it's easier to develop for now
)
target_link_libraries(PropDemo PRIVATE
	sfml-graphics
	sfml-system
	sfml-window
)

#tests
#TODO: Collect all tests and run them all
find_package(Catch2 2 REQUIRED)

#Property
add_executable(Prop_test_property
	proplib/tests/test_property.cpp
	proplib/utility/property.cpp
	proplib/utility/property.h
)
target_link_libraries(Prop_test_property PRIVATE Catch2::Catch2WithMain)

#Binding
add_executable(Prop_test_binding
	proplib/tests/test_binding.cpp
	proplib/utility/binding.cpp
	proplib/utility/binding.h
	proplib/utility/property.cpp
	proplib/utility/property.h
)
target_link_libraries(Prop_test_binding PRIVATE Catch2::Catch2WithMain)

#Polywrap
add_executable(Prop_test_polywrap
	proplib/tests/test_polywrap.cpp
	proplib/utility/polywrap.cpp
	proplib/utility/polywrap.h
)
target_link_libraries(Prop_test_polywrap PRIVATE Catch2::Catch2WithMain)

