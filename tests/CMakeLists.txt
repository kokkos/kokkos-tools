# Add an executable and its related test.
#
# The executable is always linked to 'kokkostools' and 'test_common'.
#
# Arguments:
#   TARGET_NAME       : name of the test (required)
#   SOURCE_FILE       : source file, defaults to <TARGET_NAME>.cpp (optional)
#   KOKKOS_TOOLS_LIBS : the test environment will receive the variable 'KOKKOS_TOOLS_LIBS' that is set as the path
#                       to the target file of this argument (optional)
#   KOKKOS_TOOLS_SAMPLER_VERBOSE : the test environment will receive the variable 'KOKKOS_TOOLS_SAMPLER_VERBOSE' that is set as the value of 1 for printing the sample has been taken 
#   KOKKOS_TOOLS_GLOBALFENCES : test environment receives the variable 'KOKKOS_TOOLS_GLOBALFENCES' that is set as the value of 1 to turn the tool's auto-fencing on. 
#   KOKKOS_TOOLS_RANDOM_SEED : test environment receives the variable 'KOKKOS_TOOLS_RANDOM_SEED' that is set as the value for a seed of the random number generator (used for testing repeatability). 
#   KOKKOS_TOOLS_SAMPLER_SKIP : test environment receives the variable 'KOKKOS_TOOLS_SAMPLER_SKIP' that is set as the value of the number of Kokkos kernel invocations to skip before a tooling activity is invoked. 
#   KOKKOS_TOOLS_SAMPLER_PROB : test environment receives the variable 'KOKKOS_TOOLS_SAMPLER_PROB' that is set as the probability that a Kokkos kernel invocation has a tooling activity invoked for it.


function(kp_add_executable_and_test)
	cmake_parse_arguments(kaeat_args "" "TARGET_NAME;SOURCE_FILE;KOKKOS_TOOLS_SAMPLER_VERBOSE;KOKKOS_TOOLS_GLOBALFENCES;KOKKOS_TOOLS_SAMPLER_SKIP;KOKKOS_TOOLS_SAMPLER_PROB;KOKKOS_TOOLS_RANDOM_SEED" "KOKKOS_TOOLS_LIBS" ${ARGN})
    if(NOT DEFINED kaeat_args_TARGET_NAME)
        message(FATAL_ERROR "'TARGET_NAME' is a required argument.")
    endif()

    if(NOT DEFINED kaeat_args_SOURCE_FILE)
        set(kaeat_args_SOURCE_FILE "${kaeat_args_TARGET_NAME}.cpp")
    endif()

    add_executable(${kaeat_args_TARGET_NAME})

    target_sources(
        ${kaeat_args_TARGET_NAME}
        PRIVATE
            ${kaeat_args_SOURCE_FILE}
    )
    target_link_libraries(
        ${kaeat_args_TARGET_NAME}
        PRIVATE
            kokkostools test_common
    )

    add_test(
        NAME ${kaeat_args_TARGET_NAME}
        COMMAND $<TARGET_FILE:${kaeat_args_TARGET_NAME}>
    )

    if(DEFINED kaeat_args_KOKKOS_TOOLS_LIBS)
        set(TOOL_LIBS_FILES)
        foreach(TOOL_LIB ${kaeat_args_KOKKOS_TOOLS_LIBS})
          list(APPEND TOOL_LIBS_FILES "$<TARGET_FILE:${TOOL_LIB}>")
        endforeach()
        string(REPLACE ";" "\;" TOOL_LIBS_FILES "${TOOL_LIBS_FILES}")

        set_property(
            TEST ${kaeat_args_TARGET_NAME}
            APPEND PROPERTY ENVIRONMENT "KOKKOS_TOOLS_LIBS=${TOOL_LIBS_FILES}"
        )
    endif()
    
    if(DEFINED kaeat_args_KOKKOS_TOOLS_SAMPLER_VERBOSE)
     set_property(
        TEST ${kaeat_args_TARGET_NAME}
        APPEND
        PROPERTY
        ENVIRONMENT "KOKKOS_TOOLS_SAMPLER_VERBOSE=${kaeat_args_KOKKOS_TOOLS_SAMPLER_VERBOSE}"
           )
     endif()

     if(DEFINED kaeat_args_KOKKOS_TOOLS_GLOBALFENCES)
        set_property(
        TEST ${kaeat_args_TARGET_NAME}
        APPEND
        PROPERTY
        ENVIRONMENT "KOKKOS_TOOLS_GLOBALFENCES=${kaeat_args_KOKKOS_TOOLS_GLOBALFENCES}"
               )
      endif()
     
     if (DEFINED kaeat_args_KOKKOS_TOOLS_SAMPLER_SKIP)
         set_property(
             TEST ${kaeat_args_TARGET_NAME}
             APPEND 
             PROPERTY
             ENVIRONMENT "KOKKOS_TOOLS_SAMPLER_SKIP=${kaeat_args_KOKKOS_TOOLS_SAMPLER_SKIP}"
         )
     endif()

      if (DEFINED kaeat_args_KOKKOS_TOOLS_SAMPLER_PROB)
	      set_property(
		      TEST ${kaeat_args_TARGET_NAME}
		      APPEND
		      PROPERTY
		      ENVIRONMENT "KOKKOS_TOOLS_SAMPLER_PROB=${kaeat_args_KOKKOS_TOOLS_SAMPLER_PROB}"
	      )
     endif()

     if (DEFINED kaeat_args_KOKKOS_TOOLS_RANDOM_SEED)
	     set_property(
		     TEST ${kaeat_args_TARGET_NAME}
		     APPEND
		     PROPERTY
		     ENVIRONMENT "KOKKOS_TOOLS_RANDOM_SEED=${kaeat_args_KOKKOS_TOOLS_RANDOM_SEED}"
	     )
     endif()

endfunction(kp_add_executable_and_test)

# Create a test library that contains the required Kokkos and Google Test
# initialization sequence.
add_library(test_common OBJECT)
target_sources(
    test_common
    PRIVATE
        UnitTestMain.cpp
)
target_link_libraries(test_common PUBLIC GTest::gtest GTest::gmock Kokkos::kokkos)

add_subdirectory(space-time-stack)
add_subdirectory(sampler)
