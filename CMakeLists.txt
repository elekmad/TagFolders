cmake_minimum_required(VERSION 2.8)
#Déclaration du projet
cmake_policy(SET CMP0004 OLD)
project(TagFolders)

add_library(TagFolder STATIC TagFolder.c TagFolder.h)
include_directories(TagFolder "./")

add_executable(TagFolders main.c)
target_link_libraries(TagFolders TagFolder)

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
