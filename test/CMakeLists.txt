cmake_minimum_required(VERSION 3.1)
enable_language(CXX)
set(CMAKE_CXX_STANDARD 14) # C++14...
set(CMAKE_CXX_STANDARD_REQUIRED ON) #...is required...
set(CMAKE_CXX_EXTENSIONS OFF) #...without compiler extensions like gnu++11
option(USE_RDRAND "use rdrand" ON)
option(USE_RDSEED "use rdseed" OFF)

find_package(Threads REQUIRED)

## Set our project name
project(SigContrastFastAviUtl_test)

set(SigContrastFastAviUtl_test_src
  LUT.cpp
  random.cpp
  RSigmoidTable.cpp
  SigmoidTable.cpp
  test.cpp
)
if(MSVC)
  # Force to always compile with W4
  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  endif()
  if( CMAKE_SIZEOF_VOID_P EQUAL 4 )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")
  endif()
  if(NOT USE_RDRAND)
    add_definitions(-DNO_RDRAND)
  endif()
  if(NOT USE_RDSEED)
    add_definitions(-DNO_RDSEED)
  endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUC)
  # Update if necessary
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-long-long -pedantic")
  if( CMAKE_SIZEOF_VOID_P EQUAL 4 )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32 -msse2")
  endif()
  if(USE_RDRAND)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mrdrnd")
  else()
    add_definitions(-DNO_RDRAND)
  endif()
  if(USE_RDSEED)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mrdseed")
  else()
    add_definitions(-DNO_RDSEED)
  endif()
endif()
## Define the executable
add_executable(SigContrastFastAviUtl_test ${SigContrastFastAviUtl_test_src})
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUC)
  if(WIN32)
    target_link_libraries(SigContrastFastAviUtl_test wsock32 ws2_32)
  endif()
endif()
target_link_libraries(SigContrastFastAviUtl_test Threads::Threads)
