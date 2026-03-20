CMAKE_MINIMUM_REQUIRED(VERSION 2.8.0)
PROJECT(FooDB LANGUAGES CXX)

# require c++ 17
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_EXTENSIONS OFF)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

# project setting
INCLUDE_DIRECTORIES(./src)

SET(FOODB_STORE_SOURCES
    ./src/store/bptree.cpp
    ./src/store/node.cpp)

SET(FOODB_CATALOG_SOURCES
    ./src/catalog/schema.cpp
    ./src/catalog/row.cpp
    ./src/catalog/table.cpp)

ADD_EXECUTABLE(foodb ./src/foodb.cpp ${FOODB_STORE_SOURCES} ${FOODB_CATALOG_SOURCES})
target_link_libraries(foodb fmt)

SET(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
SET(FOODB_TEST_SOURCES
    ./test/bpt_test.cpp)

FOREACH(test_file ${FOODB_TEST_SOURCES})
  STRING( REPLACE ".cpp" "" demo ${test_file})
  STRING( REPLACE "./test/" "" demo ${demo})
  MESSAGE(${demo})
  ADD_EXECUTABLE(${demo} ${test_file} ${FOODB_STORE_SOURCES} ${FOODB_CATALOG_SOURCES})
  target_link_libraries(${demo} fmt)
ENDFOREACH(test_file ${FOODB_TEST_SOURCES})
