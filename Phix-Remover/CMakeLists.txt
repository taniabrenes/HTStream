cmake_minimum_required (VERSION 3.2)
project (phix_remover)

file(GLOB sources "src/*.cpp")

message( status "sources: " ${sources})


set(COMMON_INCLUDES ${CMAKE_SOURCE_DIR}/common/src)

set(PROJECT_LINK_LIBS hts_common)
link_directories(${CMAKE_BINARY_DIR}/common)

set(Boost_USE_STATIC_LIBS   ON)
FIND_PACKAGE( Boost 1.56 COMPONENTS system program_options iostreams filesystem REQUIRED )
include_directories(${COMMON_INCLUDES} ${Boost_INCLUDE_DIR} )


add_executable(phix-remover ${sources})
target_link_libraries(phix-remover ${PROJECT_LINK_LIBS} ${Boost_LIBRARIES})

add_custom_target(valgrind_${PROJECT_NAME}
  DEPENDS ${PROJECT_NAME}
  COMMAND valgrind ./${PROJECT_NAME} -F -1 ${CMAKE_SOURCE_DIR}/test_suite/fastqFiles/testCase_1X_R1.fastq -2 ${CMAKE_SOURCE_DIR}/test_suite/fastqFiles/testCase_1X_R1.fastq -O
)

install(TARGETS phix-remover RUNTIME DESTINATION bin)

## test
set(PROJECT_TEST_NAME ${PROJECT_NAME}_test)
find_package(Threads REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS} ${Boost_INCLUDE_DIR}
    ${COMMON_INCLUDES} ${CMAKE_SOURCE_DIR}/Phix-Remover/src)

file(GLOB TEST_SRC_FILES ${PROJECT_SOURCE_DIR}/test/*.cpp)
link_directories(${GTEST_LIBS_DIR})
list(REMOVE_ITEM sources ${CMAKE_CURRENT_SOURCE_DIR}/src/phix_remover.cpp)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Dprivate=public -lboost_serialization -lboost_unit_test_framework-mt -lboost_program_options")

add_executable(${PROJECT_TEST_NAME} ${TEST_SRC_FILES} ${sources})
#add_dependencies(${PROJECT_TEST_NAME} googletest)
target_link_libraries(${PROJECT_TEST_NAME} gtest gtest_main ${CMAKE_THREAD_LIBS_INIT} ${PROJECT_LINK_LIBS})
add_test(test_phix_remover ${PROJECT_TEST_NAME})
