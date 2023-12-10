cmake_minimum_required(VERSION 3.12)

project(PropDemo VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS_DEBUG  "${CMAKE_CXX_FLAGS} -ggdb -fno-omit-frame-pointer -fsanitize=undefined,address")
set(CMAKE_LINKER_FLAGS_DEBUG  "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined,address")

#TODO: Make a prop library
set(PROP_LIBRARY_UI_NAMES
	button
	label
	vertical_layout
	widget
	window
)

set(PROP_LIBRARY_UTILITY_NAMES
	binding
	callable
	exceptions
	font
	polywrap
	property
	raii
	signal
	style
	type_list
	type_name
	type_traits
)

list(TRANSFORM PROP_LIBRARY_UI_NAMES PREPEND "proplib/ui/" OUTPUT_VARIABLE PROP_LIBRARY_UI_SOURCES)

list(TRANSFORM PROP_LIBRARY_UTILITY_NAMES PREPEND "proplib/utility/" OUTPUT_VARIABLE PROP_LIBRARY_UTILITY_SOURCES)

set(PROP_LIBRARY_SOURCES
	${PROP_LIBRARY_UI_SOURCES}
	${PROP_LIBRARY_UTILITY_SOURCES}
)
list(TRANSFORM PROP_LIBRARY_SOURCES APPEND ".cpp")

list(TRANSFORM PROP_LIBRARY_SOURCES REPLACE "\.cpp$" ".h" OUTPUT_VARIABLE PROP_LIBRARY_HEADERS)

list(TRANSFORM PROP_LIBRARY_UI_NAMES PREPEND "proplib/internals/" OUTPUT_VARIABLE PROP_LIBRARY_PRIVATE_CANDIDATES)
list(TRANSFORM PROP_LIBRARY_PRIVATE_CANDIDATES APPEND ".privates.h")
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
list(APPEND PROP_LIBRARY_TESTS_CANDIDATES ${PROP_LIBRARY_UI_NAMES} ${PROP_LIBRARY_UTILITY_NAMES})
list(TRANSFORM PROP_LIBRARY_TESTS_CANDIDATES PREPEND "proplib/tests/test_")
list(TRANSFORM PROP_LIBRARY_TESTS_CANDIDATES APPEND ".cpp")
foreach(PROP_LIBRARY_TESTS_CANDIDATE IN ITEMS ${PROP_LIBRARY_TESTS_CANDIDATES})
	if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${PROP_LIBRARY_TESTS_CANDIDATE}")
		set(PROP_LIBRARY_TESTS ${PROP_LIBRARY_TESTS} ${PROP_LIBRARY_TESTS_CANDIDATE})
	endif()
endforeach()

find_package(Catch2 2 REQUIRED)
add_executable(Prop_tests
	${PROP_LIBRARY_SOURCES}
	${PROP_LIBRARY_HEADERS}
	${PROP_LIBRARY_TESTS}
)
if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
	target_compile_options(Prop_tests PRIVATE -fconcepts-diagnostics-depth=3)
endif()
target_link_libraries(Prop_tests PRIVATE
	Catch2::Catch2WithMain
	sfml-graphics
	sfml-system
	sfml-window
)
