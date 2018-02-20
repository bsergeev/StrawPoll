cmake_minimum_required (VERSION 2.8)
set(REQUIRED_CPP_VERSION 17)

set(PROJECT_NAME StrawPollSever)
project(${PROJECT_NAME} CXX)

# Clone or update uWebSockets repo ---------------------------------------------
set(UWS_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/external/uWebSockets)
find_path(UWS_INCLUDE_DIR "uWS.h" NO_DEFAULT_PATH PATHS "${UWS_LOCATION}/src")
if (NOT UWS_INCLUDE_DIR) # The repo wasn't found => clone it
    execute_process(COMMAND git clone https://github.com/bsergeev/uWebSockets.git ${UWS_LOCATION})
else() # Pull the repo, in case it was updated
    execute_process(COMMAND git pull WORKING_DIRECTORY ${UWS_LOCATION})
endif()
# Just in case, check again
find_path(UWS_INCLUDE_DIR "uWS.h" NO_DEFAULT_PATH PATHS "${UWS_LOCATION}/src")
if (NOT UWS_INCLUDE_DIR)
    message(FATAL_ERROR "Something wrong with uWebSockets.git, aborting...")
endif()
#-------------------------------------------------------------------------------
add_subdirectory(${UWS_LOCATION})

add_executable(${PROJECT_NAME} src/server.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/strawpoll")
target_include_directories(${PROJECT_NAME} PRIVATE "${UWS_INCLUDE_DIR}")

if (MSVC)
  target_link_libraries(${PROJECT_NAME} PRIVATE uWS)
  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD ${REQUIRED_CPP_VERSION})
  set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD_REQUIRED ON)
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
elseif(UNIX)
  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++${REQUIRED_CPP_VERSION} -stdlib=libc++")
  target_link_libraries(${PROJECT_NAME} c++ c++abi pthread uWS ssl crypto z)
endif()
