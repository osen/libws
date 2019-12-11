cmake_minimum_required(VERSION 2.6)
project(WS C)

include_directories(
  ${PROJECT_SOURCE_DIR}/src
)

add_library(ws
  src/ws/ws.c
  src/ws/sha1.c
  src/ws/base64.c
  src/ws/WebSocket.c
  src/ws/TcpSocket.c
  src/ws/HttpHeader.c
)
target_link_libraries(ws)

add_executable(test_socket
  src/tests/socket.c
)
target_link_libraries(test_socket ws)

add_executable(stent_httpd
  src/tests/stent_httpd.c
)
target_link_libraries(stent_httpd ws)

add_executable(wsd
  src/wsd/main.c
)
target_link_libraries(wsd ws)
