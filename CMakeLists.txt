cmake_minimum_required(VERSION 3.12)
project(SBD2)

set(CMAKE_CXX_STANDARD 17)

add_executable(SBD2 main.cpp b_plus_tree.hh node.hh inner_node.hh leaf_node.hh tools.hh Config.hh record.cc record.hh)
target_link_libraries(SBD2 -lstdc++fs)