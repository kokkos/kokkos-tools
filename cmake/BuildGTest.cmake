# Look for Google Test and enable it as a target.
#
# The main targets that will be available are:
#       * GTest::gtest
#       * GTest::gmock
#
# References:
#   * https://github.com/google/googletest
#   * https://matgomes.com/integrate-google-test-into-cmake/
#   * https://google.github.io/googletest/quickstart-cmake.html
#   * https://jeremimucha.com/2021/04/cmake-fetchcontent/

include(FetchContent)

# Declare the Google Test dependency
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0
)

# If not yet populated, add Google Test to the build with the following options:
#       * disable installation of Google Test
#       * enable GMock
# Note that we could have used FetchContent_MakeAvailable instead, but it would then
# use the default configuration that would install Google Test.
FetchContent_GetProperties(googletest)
if (NOT googletest_POPULATED)
    FetchContent_Populate(googletest)

    set(BUILD_GMOCK   ON)
    set(INSTALL_GTEST OFF)

    add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()
