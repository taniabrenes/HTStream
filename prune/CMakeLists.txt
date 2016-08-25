cmake_minimum_required (VERSION 3.2)
project (Prune)

file(GLOB sources "src/*.cpp")

message( status "sources: " ${sources})


set(COMMON_INCLUDES ${CMAKE_SOURCE_DIR}/common/src)

set(PROJECT_LINK_LIBS hts_common)
link_directories(${CMAKE_BINARY_DIR}/common)

FIND_PACKAGE( Boost 1.56 COMPONENTS system program_options iostreams filesystem REQUIRED )
include_directories(${COMMON_INCLUDES} ${Boost_INCLUDE_DIR} )


add_executable(prune ${sources})
target_link_libraries(prune ${PROJECT_LINK_LIBS} ${Boost_LIBRARIES})

add_custom_target(testSD
  DEPENDS prune
  COMMAND ${CMAKE_COMMAND} -E env "PATH=$ENV{PATH}:${CMAKE_BINARY_DIR}/Prune" python ${CMAKE_SOURCE_DIR}/Prune/test_suite/testSD.py
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/Prune/test_suite
)

add_custom_target(valgrind
  DEPENDS prune
  COMMAND valgrind ./prune -F -1 ${CMAKE_SOURCE_DIR}/test_suite/fastqFiles/testCase_1X_R1.fastq -2 ${CMAKE_SOURCE_DIR}/test_suite/fastqFiles/testCase_1X_R1.fastq -O
)

## test
set(PROJECT_TEST_NAME ${PROJECT_NAME}_test)
find_package(Threads REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS} ${Boost_INCLUDE_DIR} ${COMMON_INCLUDES} ${CMAKE_SOURCE_DIR}/Prune/src)

file(GLOB TEST_SRC_FILES ${PROJECT_SOURCE_DIR}/test/*.cpp)
link_directories(${GTEST_LIBS_DIR})
list(REMOVE_ITEM sources ${CMAKE_CURRENT_SOURCE_DIR}/src/Prune.cpp)

