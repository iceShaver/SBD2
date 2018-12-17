cmake_minimum_required(VERSION 3.12)
project(SBD2)

set(CMAKE_CXX_STANDARD 17)
set(SOURCE_FILES b_plus_tree.hh inner_node.hh leaf_node.hh node.hh record.hh tools.hh)
add_executable(SBD2 main.cpp b_plus_tree.hh node.hh inner_node.hh leaf_node.hh tools.hh record.cc record.hh unique_generator.hh dbms.cc dbms.hh file.cc file.hh)
target_link_libraries(SBD2 -lstdc++fs -lgvc -lcdt -lcgraph -lgvpr -llab_gamut -lpathplan -lxdot -lreadline)

# set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-stack-protector")