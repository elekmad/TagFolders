if(NOT EXTERNAL_PROJECT)
    # Thanks to http://neyasystems.com/an-engineers-guide-to-unit-testing-cmake-and-boost-unit-tests/ for this part
    #Setup CMake to run tests
    option(BUILD_UNITTESTS "Create unittests target in makefile with allow to run unittests put into tests directory")
    if(BUILD_UNITTESTS)
        enable_testing()

        #I like to keep test files in a separate source directory called test
        file(GLOB TEST_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} tests/*.c)

        #Run through each source
        foreach(testSrc ${TEST_SRCS})
                #Extract the filename without an extension (NAME_WE)
                get_filename_component(testName ${testSrc} NAME_WE)

                #Add compile target
                add_executable(${testName} ${testSrc})

                #link to Boost libraries AND your targets and dependencies
                target_link_libraries(${testName} ${testLink})

                #I like to move testing binaries into a testBin directory
                set_target_properties(${testName} PROPERTIES 
                    RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_CURRENT_SOURCE_DIR}/testBin)

                #Finally add it to test execution - 
                #Notice the WORKING_DIRECTORY and COMMAND
                add_test(NAME ${testName} 
                         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/testBin 
                         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/testBin/${testName} )
        endforeach(testSrc)
    endif()
endif()
