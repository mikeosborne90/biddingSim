cmake_minimum_required(VERSION 3.12)
project(biddingSim)

set(CMAKE_CXX_STANDARD 11)

add_executable(biddingSim main.cpp biddingServer.cpp biddingClient.cpp)