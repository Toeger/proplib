cmake_minimum_required(VERSION 3.12)

project(PropDemo VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-c++26-extensions")
set(CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wno-unused-function -Wno-unused-variable -O0 -ggdb -fno-omit-frame-pointer -fsanitize=undefined,address")
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined,address")
#add_definitions(-DPROP_EMPTY_CLASS_OPTIMIZATION)
#add_definitions(-DPROPERTY_DEBUG)
add_definitions(-DPROPERTY_NAMES)

set(PROP_PLATFORM_DYNAMIC OFF)
set(PROP_PLATFORM "<unset>" CACHE STRING "Platform selection")

#TODO: Make a prop library
set(PROP_LIBRARY_UI_NAMES
	button
	combobox
	label
	vertical_layout
	widget
	window
)

set(PROP_LIBRARY_UTILITY_NAMES
	alignment
	binding
	callable
	canvas
	color
	compatibility
	dependency_tracer
	exceptions
	font
	polywrap
	property
	property_details
	raii
	rect
	signal
	style
	type_list
	type_name
	type_traits
	utility
	widget_helper
)

set(PROP_PLATFORM_NAMES
	platform.h
	platform_ansi_console.cpp
)

if (PROP_PLATFORM STREQUAL "SFML")
	list(APPEND PROP_PLATFORM_NAMES platform_sfml.cpp)
else()
	message(FATAL_ERROR "Select a platform by setting the `PROP_PLATFORM` variable, for example with `cmake -DPROP_PLATFORM=<platform>`. Valid platforms are `SFML`. `PROP_PLATFORM` is currently set to `${PROP_PLATFORM}`.")
endif()

list(TRANSFORM PROP_LIBRARY_UI_NAMES PREPEND "proplib/ui/" OUTPUT_VARIABLE PROP_LIBRARY_UI_SOURCES)
list(TRANSFORM PROP_LIBRARY_UTILITY_NAMES PREPEND "proplib/utility/" OUTPUT_VARIABLE PROP_LIBRARY_UTILITY_SOURCES)
list(TRANSFORM PROP_PLATFORM_NAMES PREPEND "proplib/platform/" OUTPUT_VARIABLE PROP_PLATFORM_NAMES)

set(PROP_LIBRARY_SOURCES
	${PROP_LIBRARY_UI_SOURCES}
	${PROP_LIBRARY_UTILITY_SOURCES}
)
list(TRANSFORM PROP_LIBRARY_SOURCES APPEND ".cpp")

list(TRANSFORM PROP_LIBRARY_SOURCES REPLACE "\.cpp$" ".h" OUTPUT_VARIABLE PROP_LIBRARY_HEADERS)

list(APPEND PROP_LIBRARY_SOURCES ${PROP_PLATFORM_NAMES})

#demo
if (PROP_PLATFORM STREQUAL "SFML")
	find_package(SFML COMPONENTS window graphics system)
endif()
add_executable(PropDemo
	demo/main.cpp
	${PROP_LIBRARY_SOURCES}
	${PROP_LIBRARY_HEADERS}
)
target_include_directories(PropDemo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if (PROP_PLATFORM STREQUAL "SFML")
	target_link_libraries(PropDemo PRIVATE
		sfml-graphics
		sfml-system
		sfml-window
	)
endif()

#experiments
if (PROP_PLATFORM STREQUAL "SFML")
	find_package(SFML COMPONENTS window graphics system)
endif()
add_executable(PropExperiments
	experiments/main.cpp
	${PROP_LIBRARY_SOURCES}
	${PROP_LIBRARY_HEADERS}
)
target_include_directories(PropExperiments PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if (PROP_PLATFORM STREQUAL "SFML")
	target_link_libraries(PropExperiments PRIVATE
		sfml-graphics
		sfml-system
		sfml-window
		-lstdc++exp
	)
endif()

#tests
list(APPEND PROP_LIBRARY_UTILITY_TESTS_CANDIDATES ${PROP_LIBRARY_UTILITY_NAMES})
list(TRANSFORM PROP_LIBRARY_UTILITY_TESTS_CANDIDATES PREPEND "proplib/tests/utility/test_")
list(TRANSFORM PROP_LIBRARY_UTILITY_TESTS_CANDIDATES APPEND ".cpp")
foreach(PROP_LIBRARY_TESTS_CANDIDATE IN ITEMS ${PROP_LIBRARY_UTILITY_TESTS_CANDIDATES})
	if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${PROP_LIBRARY_TESTS_CANDIDATE}")
		set(PROP_LIBRARY_TESTS ${PROP_LIBRARY_TESTS} ${PROP_LIBRARY_TESTS_CANDIDATE})
	endif()
endforeach()

list(APPEND PROP_LIBRARY_UI_TESTS_CANDIDATES ${PROP_LIBRARY_UI_NAMES})
list(TRANSFORM PROP_LIBRARY_UI_TESTS_CANDIDATES PREPEND "proplib/tests/ui/test_")
list(TRANSFORM PROP_LIBRARY_UI_TESTS_CANDIDATES APPEND ".cpp")
foreach(PROP_LIBRARY_TESTS_CANDIDATE IN ITEMS ${PROP_LIBRARY_UI_TESTS_CANDIDATES})
	if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${PROP_LIBRARY_TESTS_CANDIDATE}")
		set(PROP_LIBRARY_TESTS ${PROP_LIBRARY_TESTS} ${PROP_LIBRARY_TESTS_CANDIDATE})
	endif()
endforeach()

find_package(Catch2 REQUIRED)
add_executable(Prop_tests
	${PROP_LIBRARY_SOURCES}
	${PROP_LIBRARY_HEADERS}
	${PROP_LIBRARY_TESTS}
)
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
	target_compile_options(Prop_tests PRIVATE -fconcepts-diagnostics-depth=10)
endif()
target_include_directories(Prop_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if (PROP_PLATFORM STREQUAL "SFML")
	target_link_libraries(Prop_tests PRIVATE
		sfml-graphics
		sfml-system
		sfml-window
	)
endif()
target_link_libraries(Prop_tests PRIVATE
	Catch2::Catch2WithMain
	-lstdc++exp
)
