CMAKE_MINIMUM_REQUIRED(VERSION 2.8.0)
PROJECT(FooDB LANGUAGES CXX)

# require c++ 17
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_EXTENSIONS OFF)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

# project setting
INCLUDE_DIRECTORIES(./src)

AUX_SOURCE_DIRECTORY(./src/store Store)
ADD_EXECUTABLE(foodb ./src/foodb.cpp ${Store})

SET(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
AUX_SOURCE_DIRECTORY(./test TEST)

FOREACH(test_file ${TEST})
  STRING( REPLACE ".cpp" "" demo ${test_file})
  STRING( REPLACE "./test/" "" demo ${demo})
  MESSAGE(${demo})
  ADD_EXECUTABLE(${demo} ${test_file} ${Store})
ENDFOREACH(test_file ${TEST})
