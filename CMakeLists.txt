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
include_directories("/usr/include/readline")
target_link_libraries (TagFolders "/usr/lib/x86_64-linux-gnu/libhistory.a" "/usr/lib/x86_64-linux-gnu/libreadline.a")

if( WITH_COREDEBUG )
  add_definitions(-ggdb3)
  message(STATUS "GCC: Debug-flags set (-ggdb3)")
endif()
