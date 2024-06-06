//! Test setup for the 'space-time-stack' tool.
class SpaceTimeStackTest : public ::testing::Test {
 public:
  //! At the beginning of the test suite, try to add the related callbacks.
  static void SetUpTestSuite() {
    Kokkos::Tools::Experimental::set_callbacks(
        KokkosTools::get_event_set("space-time-stack", nullptr));
    Kokkos::initialize();
  }

  static void TearDownTestSuite() { Kokkos::finalize(); }

  /**
   * At test setup, finalize first and then initialize to cleanse
   * @ref KokkosTools::SpaceTimeStack::State::global_state.
   */
  void SetUp() override {
    KokkosTools::SpaceTimeStack::State::finalize();
    KokkosTools::SpaceTimeStack::State::initialize();
  }

  void TearDown() override { KokkosTools::SpaceTimeStack::State::finalize(); }
};
