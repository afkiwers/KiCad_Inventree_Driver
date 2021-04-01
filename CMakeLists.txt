cmake_minimum_required(VERSION 3.17)
project(inventree)

set(CMAKE_CXX_STANDARD 14)

find_package(wxWidgets REQUIRED COMPONENTS net core base)
include(${wxWidgets_USE_FILE})

find_package(cpprestsdk REQUIRED)


add_library(inventree SHARED inventree.cpp inventree.h IWareHouse.h)


target_link_libraries(inventree
        cpprest
        ${wxWidgets_LIBRARIES}
        )
