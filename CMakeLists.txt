cmake_minimum_required(VERSION 3.19)
project(omp-lua LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(sdk)

set(LUA_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/lua)
set(LUA_BUILD
    ${LUA_SOURCES}/lapi.c
    ${LUA_SOURCES}/lauxlib.c
    ${LUA_SOURCES}/lbaselib.c
    ${LUA_SOURCES}/lcode.c
    ${LUA_SOURCES}/lcorolib.c
    ${LUA_SOURCES}/lctype.c
    ${LUA_SOURCES}/ldblib.c
    ${LUA_SOURCES}/ldebug.c
    ${LUA_SOURCES}/ldo.c
    ${LUA_SOURCES}/ldump.c
    ${LUA_SOURCES}/lfunc.c
    ${LUA_SOURCES}/lgc.c
    ${LUA_SOURCES}/linit.c
    ${LUA_SOURCES}/liolib.c
    ${LUA_SOURCES}/llex.c
    ${LUA_SOURCES}/lmathlib.c
    ${LUA_SOURCES}/lmem.c
    ${LUA_SOURCES}/loadlib.c
    ${LUA_SOURCES}/lobject.c
    ${LUA_SOURCES}/lopcodes.c
    ${LUA_SOURCES}/loslib.c
    ${LUA_SOURCES}/lparser.c
    ${LUA_SOURCES}/lstate.c
    ${LUA_SOURCES}/lstring.c
    ${LUA_SOURCES}/lstrlib.c
    ${LUA_SOURCES}/ltable.c
    ${LUA_SOURCES}/ltablib.c
    ${LUA_SOURCES}/ltm.c
    ${LUA_SOURCES}/lundump.c
    ${LUA_SOURCES}/lutf8lib.c
    ${LUA_SOURCES}/lvm.c
    ${LUA_SOURCES}/lzio.c
)

set(LUA_HEADERS
    ${LUA_SOURCES}/lauxlib.h
    ${LUA_SOURCES}/lua.h
    ${LUA_SOURCES}/luaconf.h
    ${LUA_SOURCES}/lualib.h
)

add_library(lua STATIC ${LUA_BUILD})
if (WIN32)
    target_compile_options(lua PRIVATE /std:c99)
else()
    target_compile_options(lua PRIVATE -O2 -std=c99)
endif()

add_library(lua-header INTERFACE)
target_include_directories(lua-header INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/lua)

add_library(lua::lib ALIAS lua)
add_library(lua::header ALIAS lua-header)

add_library(${PROJECT_NAME} SHARED main.cpp)
target_link_libraries(${PROJECT_NAME} PUBLIC OMP-SDK lua::lib lua::header)
set_property(TARGET ${PROJECT_NAME} PROPERTY PREFIX "")