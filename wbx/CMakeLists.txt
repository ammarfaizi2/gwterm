cmake_minimum_required(VERSION 3.10)

project(wbx LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system)
if (NOT Boost_FOUND)
    message(FATAL_ERROR "Boost libraries not found")
endif()

find_package(OpenSSL REQUIRED)
if (NOT OpenSSL_FOUND)
    message(FATAL_ERROR "OpenSSL not found")
endif()

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../.)

set(COMMON_SOURCES
    exc/ExchangeFoundation.cpp
    exc/ExchangeFoundation.hpp
    exc/RootCerts.cpp
    exc/RootCerts.hpp
    exc/Websocket.cpp
    exc/Websocket.hpp
    exc/WebsocketImpl.cpp
    exc/WebsocketImpl.hpp
    exc/exc_binance/Binance.cpp
    exc/exc_binance/Binance.hpp
    exc/exc_okx/OKX.cpp
    exc/exc_okx/OKX.hpp)

add_library(wbx_lib STATIC ${COMMON_SOURCES})

set(CMAKE_BUILD_TYPE Release)

set(EXECUTABLES wbx pw)
add_executable(wbx entry.cpp)
add_executable(pw pw.cpp)

# Link Boost libraries
foreach(EXECUTABLE ${EXECUTABLES})
    target_link_libraries(${EXECUTABLE} wbx_lib OpenSSL::SSL OpenSSL::Crypto)
    if (WIN32)
        target_link_libraries(${EXECUTABLE} ${Boost_LIBRARIES} ws2_32)
    else()
        target_link_libraries(${EXECUTABLE} ${Boost_LIBRARIES})
    endif()

    # Compiler options (optional)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(${EXECUTABLE} PRIVATE -Wall -Wextra -pedantic -Os)
    endif()
endforeach()

message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost libraries: ${Boost_LIBRARIES}")
message(STATUS "OpenSSL include dirs: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OpenSSL libraries: ${OPENSSL_LIBRARIES}")
