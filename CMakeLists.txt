cmake_minimum_required(VERSION 3.12)
project(MyUWSApp)

set(CMAKE_CXX_STANDARD 20)

include_directories(
  ${CMAKE_SOURCE_DIR}/uWebSockets/src
  ${CMAKE_SOURCE_DIR}/uWebSockets/uSockets/src
  ${CMAKE_SOURCE_DIR}/uWebSockets/deps
)

link_directories(${CMAKE_SOURCE_DIR}/uWebSockets/build)

add_executable(my_app main.cpp)

target_link_libraries(my_app
  uwebsockets
  pthread
  ssl
  crypto
  z
)
