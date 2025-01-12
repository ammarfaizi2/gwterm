cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_COMPILER "clang++")
set(CMAKE_C_COMPILER "clang")

project(wbx LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system)
find_package(OpenSSL REQUIRED)

if (NOT Boost_FOUND)
    message(FATAL_ERROR "Boost libraries not found")
endif()

if (NOT OpenSSL_FOUND)
    message(FATAL_ERROR "OpenSSL not found")
endif()

# Include directories for Boost
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../.)

# Source files
set(SOURCES
    entry.cpp
    exc/ExchangeFoundation.cpp
    exc/ExchangeFoundation.hpp
    exc/RootCerts.cpp
    exc/RootCerts.hpp
    exc/Websocket.cpp
    exc/Websocket.hpp
    exc/WebsocketImpl.cpp
    exc/WebsocketImpl.hpp
    exc/exc_okx/OKX.cpp
    exc/exc_okx/OKX.hpp
)

set(CMAKE_BUILD_TYPE Release)

# Add the executable target
add_executable(wbx ${SOURCES})

target_link_libraries(wbx OpenSSL::SSL OpenSSL::Crypto)

# Link Boost libraries
if (WIN32)
    # Windows might require linking to ws2_32 for socket functionality
    rget_link_libraries(wbx ${Boost_LIBRARIES} ws2_32)
else()
    target_link_libraries(wbx ${Boost_LIBRARIES})
endif()

# Compiler options (optional)
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(wbx PRIVATE -Wall -Wextra -pedantic -ggdb3)
endif()

message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost libraries: ${Boost_LIBRARIES}")
message(STATUS "OpenSSL include dirs: ${OPENSSL_INCLUDE_DIR}")
message(STATUS "OpenSSL libraries: ${OPENSSL_LIBRARIES}")
