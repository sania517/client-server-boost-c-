cmake_minimum_required(VERSION 2.8)

project(Server)


set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


find_package(Boost 1.70.0 REQUIRED)

# ...

include_directories(SYSTEM ${Boost_INCLUDE_DIRS})

# ...

target_link_libraries(myapp ${Boost_LIBRARIES})

file(GLOB HEADERS *.h)
file(GLOB SOURCES *.cpp)


add_executable(Server
    ${HEADERS}
    ${SOURCES}
)



#add_executable(Server chat_message.h chat_message.cpp main.cpp)
target_link_libraries(Server pthread)
target_link_libraries(Server boost_thread)
