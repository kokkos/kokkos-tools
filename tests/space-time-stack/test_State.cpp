#include <iostream>
#include <sstream>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "Kokkos_Core.hpp"

#include "kp_all.hpp"
#include "../../profiling/space-time-stack/kp_space_time_stack.hpp"

#include "SpaceTimeStackTestSetup.hpp"

/**
 * @test Check that the @ref KokkosTools::SpaceTimeStack::State works as
 *       expected with nested pushed regions.
 */
TEST_F(SpaceTimeStackTest, nested_pushed_regions) {
  Kokkos::Profiling::pushRegion("level-0");
  Kokkos::Profiling::pushRegion("level-1");
  Kokkos::Profiling::pushRegion("level-2");
  Kokkos::Profiling::pushRegion("level-3");

  const auto& state = KokkosTools::SpaceTimeStack::State::get();

  auto check = [&](const std::string_view name,
                   const std::string_view full_name) {
    const auto& current = state.getCurrentStackFrame();
    ASSERT_EQ(current.name, name);
    ASSERT_EQ(current.get_full_name(), full_name);
    ASSERT_EQ(current.kind, KokkosTools::SpaceTimeStack::StackKind::REGION);
  };

  check("level-3", "level-0/level-1/level-2/level-3");
  Kokkos::Profiling::popRegion();

  check("level-2", "level-0/level-1/level-2");
  Kokkos::Profiling::popRegion();

  check("level-1", "level-0/level-1");
  Kokkos::Profiling::popRegion();

  check("level-0", "level-0");
  Kokkos::Profiling::popRegion();
}

template <typename view_t, typename T, typename... Args>
struct SetElementAsIndex {
  view_t view;

  KOKKOS_FUNCTION
  void operator()(const T index, Args...) const { view(index) = index; }
};

/**
 * @test Ensure that the @ref KokkosTools::SpaceTimeStack tools work as
 *       expected when there are several stack types and allocations.
 */
TEST_F(SpaceTimeStackTest, several_stack_kind) {
  using execution_space = Kokkos::DefaultExecutionSpace;
  using view_t          = Kokkos::View<int*, execution_space>;
  using policy_t        = Kokkos::RangePolicy<Kokkos::DefaultExecutionSpace>;
  using index_t         = typename policy_t::index_type;

  constexpr size_t size = 1;

  Kokkos::Profiling::pushRegion("testing");

  view_t data("my data", size);

  Kokkos::parallel_scan(
      "initialize my data values", policy_t(0, size),
      SetElementAsIndex<view_t, index_t, index_t&, const bool>{data});

  view_t copy("my copy of data", size);
  Kokkos::deep_copy(copy, data);

  const auto& state = KokkosTools::SpaceTimeStack::State::get();

  const auto& current = state.getCurrentStackFrame();

  ASSERT_EQ(current.children.size(), 3);

  auto child = current.children.cbegin();

  ASSERT_EQ(child->name,
            "Kokkos::View::initialization [my copy of data] via memset");
  ASSERT_EQ(child->kind, KokkosTools::SpaceTimeStack::StackKind::FOR);

  ++child;

  ASSERT_EQ(child->name, "Kokkos::View::initialization [my data] via memset");
  ASSERT_EQ(child->kind, KokkosTools::SpaceTimeStack::StackKind::FOR);

  ++child;

  ASSERT_EQ(child->name, "initialize my data values");
  ASSERT_EQ(child->kind, KokkosTools::SpaceTimeStack::StackKind::SCAN);

  const std::string memory_space_name(execution_space::memory_space::name());
  const auto space_as_int = KokkosTools::SpaceTimeStack::get_space(
      Kokkos::Tools::make_space_handle(memory_space_name.c_str()));

  const auto& hwm_allocs = state.getHighWaterMemAllocs(space_as_int);
  ASSERT_EQ(hwm_allocs.alloc_set.size(), 2);

  Kokkos::Profiling::popRegion();
}
