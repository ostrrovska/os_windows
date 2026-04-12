cmake_minimum_required(VERSION 4.1)
project(lab4_3fact_)

set(CMAKE_CXX_STANDARD 20)

add_executable(lab4_3fact_
        main5.cpp)

target_link_libraries(lab4_3fact_ PRIVATE comctl32)
