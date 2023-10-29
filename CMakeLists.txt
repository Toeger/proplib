cmake_minimum_required(VERSION 3.5)

project(Prop VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        main.cpp
		property.cpp
		raii.cpp
)

add_executable(Prop ${PROJECT_SOURCES})

find_package(Catch2 2 REQUIRED)
add_executable(tests test_property.cpp property.cpp)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)

target_link_libraries(Prop PRIVATE Qt6::Widgets)
