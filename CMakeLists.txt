cmake_minimum_required(VERSION 3.12)
project(mylibrary)

# config
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# set some value
set(VCPKG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/3rd/vcpkg")
set(
    3RD_LIST
    cjson
)

if(WIN32)
    set(VCPKG_BOOTSTRAP_SCRIPT "bootstrap-vcpkg.bat")
    set(VCPKG_EXEC "vcpkg.exe")
else()
    set(VCPKG_BOOTSTRAP_SCRIPT "bootstrap-vcpkg.sh")
    set(VCPKG_EXEC "vcpkg")
endif()

# config vcpkg
if(NOT EXISTS "${VCPKG_DIR}")
    message(FATAL_ERROR "vcpkg directory not exist at: ${VCPKG_DIR}")
endif()

if(NOT EXISTS "${VCPKG_DIR}/${VCPKG_BOOTSTRAP_SCRIPT}")
    message(FATAL_ERROR "The vcpkg bootstrap script does not exist: ${VCPKG_DIR}/${VCPKG_BOOTSTRAP_SCRIPT}")
endif()

execute_process(
    COMMAND ${VCPKG_DIR}/${VCPKG_BOOTSTRAP_SCRIPT}
    RESULT_VARIABLE VCPKG_BOOTSTRAP_RESULT
)

if(NOT VCPKG_BOOTSTRAP_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to bootstrap vcpkg")
endif()

# install 3rd libraries
foreach(pkg ${3RD_LIST})
execute_process(
    COMMAND ${VCPKG_DIR}/${VCPKG_EXEC} install ${pkg}
    RESULT_VARIABLE VCPKG_INSTALL_RESULT
)
if(NOT VCPKG_INSTALL_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to install libraries using vcpkg")
else()
    message("install ${pkg} done.")
endif()
endforeach(pkg ${3RD_LIST})

# interface
include_directories(${CMAKE_SOURCE_DIR}/interface)
file(GLOB INTERFACE "${CMAKE_SOURCE_DIR}/interface/*.h")

if(WIN32)
    include_directories(${CMAKE_SOURCE_DIR}/3rd/vcpkg/installed/x64-windows/include)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        link_directories(${CMAKE_SOURCE_DIR}/3rd/vcpkg/installed/x64-windows/lib)
        file(GLOB VCPKG_LIBS "${CMAKE_SOURCE_DIR}/3rd/vcpkg/installed/x64-windows/lib/*.lib")
    else()
        link_directories(${CMAKE_SOURCE_DIR}/3rd/vcpkg/installed/x64-windows/debug/lib)
        file(GLOB VCPKG_LIBS "${CMAKE_SOURCE_DIR}/3rd/vcpkg/installed/x64-windows/debug/lib/*.lib")
    endif()
endif()

# source
file(GLOB HEADER "${CMAKE_SOURCE_DIR}/src/*.h")
file(GLOB SOURCE "${CMAKE_SOURCE_DIR}/src/*.cpp")

add_library(ygomasterArchiveMgr ${INTERFACE} ${HEADER} ${SOURCE})
source_group("interface" FILES ${INTERFACE})
source_group("header" FILES ${HEADER})
source_group("source" FILES ${SOURCE})

# target_link_libraries(mylibrary.dll ${3rd_lib_name})

add_executable(YgoMasterArchiveTool demo/main.cpp)
target_link_libraries(YgoMasterArchiveTool ygomasterArchiveMgr)