cmake_minimum_required (VERSION 3.2)
project (Super-Deduper)

file(GLOB sources "src/*.cpp")

message( status "sources: " ${sources})


set(COMMON_INCLUDES ${CMAKE_SOURCE_DIR}/common/src)

set(PROJECT_LINK_LIBS hts_common)
link_directories(${CMAKE_BINARY_DIR}/common)

FIND_PACKAGE( Boost 1.56 COMPONENTS system program_options iostreams filesystem REQUIRED )
include_directories(${COMMON_INCLUDES} ${Boost_INCLUDE_DIR} )


add_executable(prune ${sources})
target_link_libraries(prune ${PROJECT_LINK_LIBS} ${Boost_LIBRARIES})


## test
set(PROJECT_TEST_NAME ${PROJECT_NAME}_test)
find_package(Threads REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS} ${Boost_INCLUDE_DIR} ${COMMON_INCLUDES} ${CMAKE_SOURCE_DIR}/prune/src)

file(GLOB TEST_SRC_FILES ${PROJECT_SOURCE_DIR}/test/*.cpp)
link_directories(${GTEST_LIBS_DIR})
list(REMOVE_ITEM sources ${CMAKE_CURRENT_SOURCE_DIR}/src/prune.cpp)