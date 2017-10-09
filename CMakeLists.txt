cmake_minimum_required(VERSION 2.8)
#Déclaration du projet
cmake_policy(SET CMP0004 OLD)
project(TagFolders)
include(ExternalProject)

set(STRINGS "${CMAKE_CURRENT_BINARY_DIR}/String")

#Setup remote project libString
option(UPDATE_REMOTE "Perform git up on remote project")
set(DISCONNECT "1")
if(${UPDATE_REMOTE})
    message(STATUS "Will perform git up on remote project")
    set(DISCONNECT "0")
endif()

externalproject_add(string_project
       GIT_REPOSITORY https://github.com/elekmad/String.git
       PREFIX "${STRINGS}"
       INSTALL_COMMAND ""
       CMAKE_ARGS "-DEXTERNAL_PROJECT=1"
       UPDATE_DISCONNECTED ${DISCONNECT}
)

add_library(TagFolder STATIC TagFolder.c TagFolder.h)
include_directories(TagFolder "${CMAKE_CURRENT_BINARY_DIR}" "${STRINGS}/src/string_project/")
target_link_libraries(TagFolder "-L${STRINGS}/src/string_project-build/ -lStrings")

set(testLink TagFolder)
# add unit tests in tests directory (make test to run)
include(unittests.cmake)
# add dpoxygen functionality
#include(doxygen.cmake)

#add_executable(TagFolders main.c)
#target_link_libraries(TagFolders TagFolder)

#Déclaration de l'exécutable

target_link_libraries (TagFolder "-lsqlite3")
#include_directories("-I/usr/include/readline")
#target_link_libraries (TagFolders "-L/usr/local/lib -lreadline")

find_package(OpenSSL)
if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
    target_link_libraries (TagFolder ${OPENSSL_LIBRARIES})
endif (OPENSSL_FOUND)

if( WITH_COREDEBUG )
  add_definitions(-ggdb3)
  message(STATUS "GCC: Debug-flags set (-ggdb3)")
endif()
