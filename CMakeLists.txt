cmake_minimum_required(VERSION 2.6)
project(WS)

include_directories(
  ${PROJECT_SOURCE_DIR}/src
)

add_library(ws
  src/ws/ws.c
  src/ws/base64.c
  src/ws/sha1.c
  src/ws/handshake.c
)

add_executable(simple
  src/simple/main.c
)

target_link_libraries(simple ws)

