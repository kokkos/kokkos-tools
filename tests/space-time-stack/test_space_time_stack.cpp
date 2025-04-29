#include <iostream>

#include "Kokkos_Core.hpp"
#include "kp_all.hpp"

/**
 * @test This test ensures that the tool uses demangled kernel names.
 */
void test_demangling(const std::string& /* unused */, const size_t size) {
    //! Explicitly launch a kernel with no name, to stress the demangling feature.
    Kokkos::parallel_for(
        size,
        KOKKOS_LAMBDA(const size_t /* unused */){}
    );

    /// The goal here would be to directly look at the struct @c State
    /// defined in @c kp_space_time_stack.cpp to make meaningful test assertions.
}


int main(int argc, char *argv[]) {

    //! Retrieve available callbacks.
    auto eventSet = KokkosTools::get_event_set("space-time-stack", "");

    //! @note Callbacks must be set before @c Kokkos::Initialize.
    Kokkos::Tools::Experimental::set_callbacks(eventSet);
    Kokkos::initialize(argc, argv);

    //! Run tests. @todo Replace this with Google Test.
    test_demangling("", 1);

    Kokkos::finalize();

    return EXIT_SUCCESS;
}
