cmake_minimum_required(VERSION 2.8)
#Déclaration du projet
cmake_policy(SET CMP0004 OLD)
project(TagFolders)
include(ExternalProject)

set(STRINGS "${CMAKE_CURRENT_BINARY_DIR}/String")

externalproject_add(string_project
       GIT_REPOSITORY git@github.com:elekmad/String.git
       PREFIX "${STRINGS}"
       INSTALL_COMMAND ""
       CMAKE_ARGS "-DEXTERNAL_PROJECT=1"
)

add_library(TagFolder STATIC TagFolder.c TagFolder.h)
include_directories(TagFolder "${CMAKE_CURRENT_BINARY_DIR}" "${STRINGS}/src/string_project/")

add_executable(TagFolders main.c)
target_link_libraries(TagFolders TagFolder)
target_link_libraries(TagFolders "-L${STRINGS}/src/string_project-build/ -lStrings")

#Déclaration de l'exécutable

target_link_libraries (TagFolders "-lsqlite3")
include_directories("-I/usr/include/readline")
target_link_libraries (TagFolders "-L/usr/local/lib -lreadline")

find_package(OpenSSL)
if(OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIR})
    target_link_libraries (TagFolders ${OPENSSL_LIBRARIES})
endif (OPENSSL_FOUND)

if( WITH_COREDEBUG )
  add_definitions(-ggdb3)
  message(STATUS "GCC: Debug-flags set (-ggdb3)")
endif()
