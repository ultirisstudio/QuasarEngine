cmake_minimum_required(VERSION 3.20)
project(lua C)

add_library(lua STATIC)

file(GLOB LUA_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

list(FILTER LUA_SOURCES EXCLUDE REGEX ".*/lua\\.c$")
list(FILTER LUA_SOURCES EXCLUDE REGEX ".*/luac\\.c$")

target_sources(lua PRIVATE ${LUA_SOURCES})

target_include_directories(lua PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

if(WIN32)
  target_compile_definitions(lua PRIVATE LUA_USE_WINDOWS)
endif()

set_target_properties(lua PROPERTIES FOLDER "Dependencies/lua")
