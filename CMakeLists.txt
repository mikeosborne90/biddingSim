cmake_minimum_required(VERSION 3.12)
project(biddingSim)

set(CMAKE_CXX_STANDARD 11)

add_executable(biddingSim main.cpp biddingServer.cpp biddingClient1.cpp biddingClient2.cpp biddingClient3.cpp biddingClient4.cpp)